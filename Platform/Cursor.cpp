// The mouse cursor: Windows .cur / .ani files on SDL cursors. See Platform/Cursor.h.
#include "Platform/Cursor.h"
#include "Platform/WindowsAPI.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace PlatformCursor {

namespace {

inline unsigned rd16(const unsigned char* p){ return unsigned(p[0]) | (unsigned(p[1]) << 8); }
inline unsigned rd32(const unsigned char* p){ return rd16(p) | (rd16(p + 2) << 16); }

/// One decoded cursor, and the timing that walks it. A plain .cur has a single frame
/// and no steps; an .ani has one frame per "icon" chunk and one step per entry in its
/// rate list -- the two differ whenever the file carries a "seq " chunk, which reorders
/// and repeats frames.
class Cursor
{
public:
	Cursor() : period_(0) {}
	~Cursor()
	{
		for(size_t i = 0; i < frames_.size(); ++i)
			SDL_DestroyCursor(frames_[i]);
	}

	bool empty() const { return frames_.empty(); }
	unsigned period() const { return period_; }
	int frameCount() const { return int(frames_.size()); }
	int stepCount() const { return int(delays_.size()); }
	unsigned delay(int step) const { return delays_[step]; }

	/// The SDL cursor a step shows. A file with no step list has the one frame.
	SDL_Cursor* frame(int step) const
	{
		return frames_[step >= 0 && step < int(steps_.size()) ? steps_[step] : 0];
	}

	void addFrame(SDL_Cursor* frame){ frames_.push_back(frame); }
	void addStep(int frame, unsigned delayMs)
	{
		steps_.push_back(frame >= 0 && frame < int(frames_.size()) ? frame : 0);
		delays_.push_back(delayMs);
		period_ += delayMs;
	}

private:
	Cursor(const Cursor&);
	Cursor& operator=(const Cursor&);

	std::vector<SDL_Cursor*> frames_;
	std::vector<int>         steps_;   ///< step -> frame index
	std::vector<unsigned>    delays_;  ///< step -> milliseconds
	unsigned                 period_;  ///< sum of delays_
};

/// The cursor currently showing. SDL holds one image per cursor, so walking an
/// animation is ours to do; this owns the clock that does it, and the knowledge of
/// which frame is up.
class ActiveCursor
{
public:
	static ActiveCursor& instance()
	{
		static ActiveCursor active;
		return active;
	}

	void set(Cursor* cursor)
	{
		if(cursor && cursor->empty())
			cursor = 0;
		if(applied_ && cursor == cursor_)   // unchanged: leave the animation clock alone
			return;

		applied_ = true;
		cursor_ = cursor;
		step_ = -1;
		since_ = SDL_GetTicks();

		if(!cursor_){
			SDL_HideCursor();
			return;
		}
		apply(0);
		SDL_ShowCursor();
	}

	/// Advance to the frame the rate list says the clock has reached.
	void animate()
	{
		if(!cursor_ || cursor_->stepCount() < 2 || !cursor_->period())
			return;
		// The cursor is a video API. The load calls this from wherever it happens to
		// be running, so let a worker thread skip its turn rather than reach for it.
		if(!SDL_IsMainThread())
			return;

		const unsigned now = unsigned((SDL_GetTicks() - since_) % cursor_->period());

		int step = 0;
		unsigned elapsed = 0;
		for(; step < cursor_->stepCount() - 1; ++step){
			elapsed += cursor_->delay(step);
			if(now < elapsed)
				break;
		}

		if(step != step_)
			apply(step);
	}

	/// Called when the cursor showing is about to be destroyed.
	void forget(const Cursor* cursor)
	{
		if(cursor_ != cursor)
			return;
		cursor_ = 0;
		step_ = -1;
		SDL_SetCursor(SDL_GetDefaultCursor());
	}

private:
	ActiveCursor() : cursor_(0), since_(0), step_(-1), applied_(false) {}

	void apply(int step)
	{
		step_ = step;
		SDL_SetCursor(cursor_->frame(step));
	}

	Cursor* cursor_;
	Uint64  since_;
	int     step_;
	bool    applied_;   ///< whether set() has ever run: the first call must not be skipped
};

// One image out of an ICO/CUR directory: a BITMAPINFOHEADER, the colour bitmap, and a
// 1-bit AND mask beneath it -- which is why the header's height is twice the real one.
// Both bitmaps are bottom-up with rows padded to four bytes. The game's files are 24-bit
// (the animated frames) and 8-bit paletted (the static ones); 1, 4 and 32 are here
// because the format allows them and cost a line each.
SDL_Surface* decodeDIB(const unsigned char* img, unsigned size)
{
	const unsigned headerSize = size >= 40 ? rd32(img) : 0;
	if(headerSize < 40 || headerSize > size)   // a PNG-compressed entry (Vista+) fails here
		return 0;

	const int width  = int(rd32(img + 4));
	const int height = int(rd32(img + 8)) / 2;
	const unsigned bpp = rd16(img + 14);
	if(width <= 0 || height <= 0 || rd32(img + 16) != 0)   // BI_RGB only
		return 0;
	if(bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32)
		return 0;

	unsigned paletteSize = 0;
	if(bpp <= 8){
		paletteSize = rd32(img + 32);          // biClrUsed, 0 meaning the whole table
		if(!paletteSize || paletteSize > (1u << bpp))
			paletteSize = 1u << bpp;
	}
	const unsigned char* palette = img + headerSize;
	const unsigned char* colour  = palette + paletteSize * 4;

	const unsigned colourStride = ((unsigned(width) * bpp + 31) / 32) * 4;
	const unsigned maskStride   = ((unsigned(width) + 31) / 32) * 4;
	const unsigned char* mask   = colour + colourStride * unsigned(height);

	if(mask + maskStride * unsigned(height) > img + size)
		return 0;

	// 32-bit entries carry their own alpha, but the ones written by tools that only
	// ever filled the mask leave it zero throughout. Fall back to the mask there,
	// rather than building an entirely transparent cursor.
	bool hasAlpha = false;
	if(bpp == 32)
		for(unsigned i = 3; i < colourStride * unsigned(height) && !hasAlpha; i += 4)
			hasAlpha = colour[i] != 0;

	SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
	if(!surface)
		return 0;

	for(int y = 0; y < height; ++y){
		const unsigned char* src  = colour + colourStride * unsigned(height - 1 - y);
		const unsigned char* mrow = mask   + maskStride   * unsigned(height - 1 - y);
		Uint32* dst = (Uint32*)((unsigned char*)surface->pixels + surface->pitch * y);

		for(int x = 0; x < width; ++x){
			unsigned r, g, b, a = 255;
			if(bpp == 32 || bpp == 24){
				const unsigned char* p = src + x * (bpp / 8);
				b = p[0]; g = p[1]; r = p[2];
				if(bpp == 32 && hasAlpha)
					a = p[3];
			}
			else {
				unsigned index;
				if(bpp == 8)      index = src[x];
				else if(bpp == 4) index = (src[x >> 1] >> ((x & 1) ? 0 : 4)) & 0x0F;
				else              index = (src[x >> 3] >> (7 - (x & 7))) & 1;
				if(index >= paletteSize)
					index = 0;
				b = palette[index * 4]; g = palette[index * 4 + 1]; r = palette[index * 4 + 2];
			}
			if(bpp != 32 || !hasAlpha)
				a = ((mrow[x >> 3] >> (7 - (x & 7))) & 1) ? 0 : 255;

			dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	return surface;
}

// A whole ICO/CUR file: its first (here, only) image, and the hotspot the directory
// entry carries. Type 1 is an icon and has none -- two of the game's "cursors" are
// icons, Empty and Camera -- so those point at their own top-left corner, which is
// where Windows put them too.
SDL_Cursor* decodeIcon(const unsigned char* data, unsigned size)
{
	if(size < 22 || rd16(data) != 0)
		return 0;
	const unsigned type = rd16(data + 2);
	if((type != 1 && type != 2) || rd16(data + 4) < 1)
		return 0;

	const unsigned char* entry = data + 6;
	const unsigned bytes  = rd32(entry + 8);
	const unsigned offset = rd32(entry + 12);
	if(offset >= size || bytes > size - offset)
		return 0;

	SDL_Surface* surface = decodeDIB(data + offset, bytes);
	if(!surface)
		return 0;

	int hotX = 0, hotY = 0;
	if(type == 2){
		hotX = int(rd16(entry + 4));
		hotY = int(rd16(entry + 6));
		if(hotX >= surface->w) hotX = surface->w - 1;
		if(hotY >= surface->h) hotY = surface->h - 1;
	}

	SDL_Cursor* cursor = SDL_CreateColorCursor(surface, hotX, hotY);
	SDL_DestroySurface(surface);
	return cursor;
}

// RIFF/ACON: an "anih" header, an optional "rate" (per step, in 1/60s jiffies) and
// "seq " (step -> frame), and a LIST/fram holding one "icon" chunk per frame, each a
// complete .cur file of its own. Chunks are word-aligned.
void decodeAni(const unsigned char* data, unsigned size, Cursor& out)
{
	unsigned steps = 0, defaultJiffies = 0;
	std::vector<unsigned> rate;
	std::vector<int> seq;

	unsigned p = 12;
	while(p + 8 <= size){
		const unsigned char* id = data + p;
		unsigned len = rd32(data + p + 4);
		p += 8;
		if(len > size - p)
			len = size - p;

		if(!memcmp(id, "anih", 4) && len >= 36){
			steps = rd32(data + p + 8);
			defaultJiffies = rd32(data + p + 28);
		}
		else if(!memcmp(id, "rate", 4)){
			for(unsigned i = 0; i + 4 <= len; i += 4)
				rate.push_back(rd32(data + p + i));
		}
		else if(!memcmp(id, "seq ", 4)){
			for(unsigned i = 0; i + 4 <= len; i += 4)
				seq.push_back(int(rd32(data + p + i)));
		}
		else if(!memcmp(id, "LIST", 4) && len >= 4 && !memcmp(data + p, "fram", 4)){
			unsigned q = p + 4;
			while(q + 8 <= p + len){
				const unsigned char* sid = data + q;
				unsigned slen = rd32(data + q + 4);
				q += 8;
				if(slen > p + len - q)
					slen = p + len - q;
				if(!memcmp(sid, "icon", 4))
					if(SDL_Cursor* frame = decodeIcon(data + q, slen))
						out.addFrame(frame);
				q += slen + (slen & 1);
			}
		}

		p += len + (len & 1);
	}

	if(out.empty())
		return;

	if(!steps)
		steps = unsigned(out.frameCount());   // no header: one step per frame
	for(unsigned i = 0; i < steps; ++i){
		unsigned jiffies = i < rate.size() ? rate[i] : defaultJiffies;
		if(!jiffies)
			jiffies = 1;
		out.addStep(i < seq.size() ? seq[i] : int(i), jiffies * 1000 / 60);
	}
}

} // namespace

Handle load(const char* fileName)
{
	if(!fileName || !*fileName)
		return 0;

	if(!SDL_WasInit(SDL_INIT_VIDEO)){
		fprintf(stderr, "PlatformCursor: no video yet, cannot load %s\n", fileName);
		return 0;
	}

	// LoadImage read the path straight off disk and so does this -- the cursors are
	// loose files, not archive members. Off-Windows the engine's backslash path has to
	// become a real one first, case and all (Platform/WindowsAPI.h).
#ifdef _WIN32
	const std::string path(fileName);
#else
	const std::string path = NormalizePath(fileName);
#endif

	FILE* file = fopen(path.c_str(), "rb");
	if(!file){
		fprintf(stderr, "PlatformCursor: cannot open %s\n", fileName);
		return 0;
	}
	fseek(file, 0, SEEK_END);
	const long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<unsigned char> data(length > 0 ? size_t(length) : 0);
	if(data.empty() || fread(&data[0], 1, data.size(), file) != data.size())
		data.clear();
	fclose(file);

	Cursor* cursor = new Cursor;
	if(data.size() >= 12 && !memcmp(&data[0], "RIFF", 4) && !memcmp(&data[8], "ACON", 4))
		decodeAni(&data[0], unsigned(data.size()), *cursor);
	else if(data.size() >= 22)
		if(SDL_Cursor* one = decodeIcon(&data[0], unsigned(data.size())))
			cursor->addFrame(one);

	if(cursor->empty()){
		fprintf(stderr, "PlatformCursor: %s is not a cursor this can read\n", fileName);
		delete cursor;
		return 0;
	}

	return cursor;
}

void destroy(Handle handle)
{
	Cursor* cursor = (Cursor*)handle;
	if(!cursor)
		return;

	ActiveCursor::instance().forget(cursor);
	delete cursor;
}

void set(Handle handle)
{
	ActiveCursor::instance().set((Cursor*)handle);
}

void animate()
{
	ActiveCursor::instance().animate();
}

} // namespace PlatformCursor
