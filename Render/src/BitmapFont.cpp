#include "StdAfxRD.h"
#include "FT_Font.h"
#include "FileRead.h"
#include "TexLibrary.h"
#include "Texture.h"
#include <math.h>
#include <vector>

void dprintfW(const wchar_t *format, ...);

// Same gap between glyphs in the atlas as the FreeType path uses (FT_Font.cpp).
#define GLYPH_PAD_SPACE 2

// Maelstrom's own font format, for running its data on this engine.
//
// It shipped no TrueType at all: its fonts are 1-bit-per-pixel bitmap masters, one file
// per face, rasterised offline at a fixed height (103-110 px) and scaled down to whatever
// size the UI asks for.  The reader below is the same one the original had, in
// Render/src/Font.cpp::LoadFontImage:
//
//     "font" | int32 real_height | uint16 char_min | uint16 char_max
//     per char in [char_min, char_max):
//         int32 width | uint8 bits[((width + 7) / 8) * real_height]
//
// Rows run top to bottom, most significant bit first.  All 15 masters in the distribution
// parse with every byte consumed and nothing left over.
//
// The *.xfont files beside them are not the source: cFontInternal::CreateTexture tries
// Load(name) first and, failing that, builds from the master and writes the .xfont back.
// They are a cache of six sizes that machine happened to ask for, so they are ignored here.
//
// The result is an ordinary FT::Font -- same atlas texture, same charTable_, same metrics --
// so nothing downstream of this file knows the difference.

// Perimeter 2 has no use for any of this -- it ships TrueType -- so the whole file is a
// Maelstrom branch and compiles to nothing in a stock build.
#ifdef MAELSTROM_DATA

namespace FT {

namespace {

const int MASTER_FIRST_CHAR = 32;

struct Master
{
	int height;
	int lo, hi;
	std::vector<int> width;                    // indexed by byte - lo
	std::vector<std::vector<uint8> > bits;     // ditto; ((width+7)/8) * height each

	bool has(int byteCode) const { return byteCode >= lo && byteCode < hi; }
	int  widthOf(int byteCode) const { return width[byteCode - lo]; }

	// One pixel of the master, as 0 or 1.
	int pixel(int byteCode, int x, int y) const
	{
		const std::vector<uint8>& b = bits[byteCode - lo];
		int rowBytes = (widthOf(byteCode) + 7) / 8;
		int bit = x + y * rowBytes * 8;
		return (b[bit >> 3] & (1 << ((7 - bit) & 7))) ? 1 : 0;
	}
};

bool loadMaster(const char* path, Master& m)
{
	char* buf = 0;
	int size = 0;
	if(!RenderFileRead(path, buf, size))
		return false;

	bool ok = false;
	do {
		if(size < 12 || memcmp(buf, "font", 4))
			break;

		const uint8* p = reinterpret_cast<const uint8*>(buf);
		int off = 4;
		memcpy(&m.height, p + off, 4); off += 4;
		uint16 lo = 0, hi = 0;
		memcpy(&lo, p + off, 2); off += 2;
		memcpy(&hi, p + off, 2); off += 2;
		m.lo = lo; m.hi = hi;

		if(m.height <= 0 || m.height > 512 || m.lo >= m.hi || m.hi > 256)
			break;

		m.width.resize(m.hi - m.lo);
		m.bits.resize(m.hi - m.lo);

		int i = 0;
		for(; i < m.hi - m.lo; ++i){
			if(off + 4 > size)
				break;
			int w = 0;
			memcpy(&w, p + off, 4); off += 4;
			if(w < 0 || w > 4096)
				break;
			int bytes = ((w + 7) / 8) * m.height;
			if(off + bytes > size)
				break;
			m.width[i] = w;
			m.bits[i].assign(p + off, p + off + bytes);
			off += bytes;
		}
		ok = (i == m.hi - m.lo);
	} while(0);

	delete[] buf;
	return ok;
}

// Which codepage a master's byte indices are in.  It is not one answer for the whole
// distribution: byte 0xC0 draws a plain "А" in Russian/MAEL_small.font and an accented "À"
// in English/MAEL_small.font, so the LocData language directory decides.  The developers'
// own faces under Scripts/Resource/fonts are Cyrillic, hence the 1251 default.
uint16 codepageOf(const char* path)
{
	static const char* western[] = { "english", "german", "french", "spanish", "italian" };

	string lower(path ? path : "");
	for(size_t i = 0; i < lower.size(); ++i)
		lower[i] = tolower(static_cast<unsigned char>(lower[i]));

	for(size_t i = 0; i < sizeof(western) / sizeof(western[0]); ++i){
		string dir = string("locdata\\") + western[i];
		string alt = string("locdata/") + western[i];
		if(lower.find(dir) != string::npos || lower.find(alt) != string::npos)
			return 1252;
	}
	return 1251;
}

// utf16 -> the byte that means it in `page`, or 0 when the page cannot spell it.  Built by
// the same MultiByteToWideChar the char table itself was built with, so the two agree by
// construction rather than by a hand-copied table.
void buildReverseMap(uint16 page, std::vector<uint8>& rev)
{
	rev.assign(0x10000, 0);
	for(int code = MASTER_FIRST_CHAR; code < 0x100; ++code){
		char ch = static_cast<char>(code);
		WCHAR wch[2] = { 0, 0 };
		if(MultiByteToWideChar(page, MB_PRECOMPOSED, &ch, 1, wch, 2) > 0 && wch[0])
			if(!rev[wch[0]])
				rev[wch[0]] = static_cast<uint8>(code);
	}
}

} // namespace

// The master is scaled by mul = requested / real_height and the *whole cell* comes with it,
// padding included -- which is what the original did (cFontInternal::CreateImage), so the
// glyphs sit at the same fraction of their nominal size as they did there.  Trimming to the
// ink would give a visually larger font than Maelstrom shipped.
Font* FontManager::createBitmapFont(const char* path, uint8 pixelSize, const FontParam* prm)
{
	Master master;
	if(!loadMaster(path, master))
		return 0;

	if(pixelSize < 1)
		return 0;

	const float mul = pixelSize / float(master.height);

	std::vector<uint8> rev;
	buildReverseMap(codepageOf(path), rev);

	// Scaled cell width per char table entry, and the atlas big enough to hold them.
	const int count = static_cast<int>(chars_.size());
	std::vector<int> cellW(count, 0), byteOf(count, 0);
	for(int i = 0; i < count; ++i){
		uint16 utf = chars_[i];
		int b = rev[utf];
		if(!b || !master.has(b))
			continue;
		byteOf[i] = b;
		cellW[i] = max(1, int(master.widthOf(b) * mul + 0.5f));
		if(cellW[i] > 255 - GLYPH_PAD_SPACE)
			cellW[i] = 255 - GLYPH_PAD_SPACE;
	}

	ShortSize size;
	size.x = size.y = 128;
	for(;;){
		int x = GLYPH_PAD_SPACE, y = GLYPH_PAD_SPACE;
		for(int i = 0; i < count; ++i){
			if(!cellW[i])
				continue;
			if(x + GLYPH_PAD_SPACE + cellW[i] > size.x){
				x = GLYPH_PAD_SPACE;
				y += pixelSize + GLYPH_PAD_SPACE;
			}
			x += cellW[i] + GLYPH_PAD_SPACE;
		}
		if(y + pixelSize + GLYPH_PAD_SPACE <= size.y)
			break;
		if(size.x <= size.y)
			size.x *= 2;
		else
			size.y *= 2;
		if(size.x > 2048 || size.y > 2048){
			dprintfW(L"BitmapFont: %S at %d does not fit 2048x2048\n", path, pixelSize);
			return 0;
		}
	}

	Font* font = new Font();
	font->param_ = *prm;
	font->size_ = pixelSize;
	font->lineHeight_ = pixelSize;

	if(!(font->texture_ = GetTexLibrary()->CreateAlphaTexture(size.x, size.y))){
		delete font;
		return 0;
	}

	int pitch = 0;
	BYTE* buf = font->texture_->LockTexture(pitch);
	if(!buf){
		releaseFont(font);
		return 0;
	}
	memset(buf, 0, size.y * pitch);

	uint16 x = GLYPH_PAD_SPACE, y = GLYPH_PAD_SPACE;
	for(int i = 0; i < count; ++i){
		font->charTable_.push_back(OneChar());
		OneChar& one = font->charTable_.back();
		if(!cellW[i])
			continue;

		if(x + GLYPH_PAD_SPACE + cellW[i] > size.x){
			x = GLYPH_PAD_SPACE;
			y += pixelSize + GLYPH_PAD_SPACE;
		}

		// Box filter from the master into the cell: every destination pixel averages the
		// master pixels it covers.  That averaging is the whole of the antialiasing --
		// the source is 1 bit deep, and the original got its greys the same way, by
		// rendering at master resolution and resampling down.
		const int b = byteOf[i];
		const int sw = master.widthOf(b), sh = master.height;
		for(int dy = 0; dy < pixelSize; ++dy){
			const int y0 = dy * sh / pixelSize, y1 = max(y0 + 1, (dy + 1) * sh / pixelSize);
			for(int dx = 0; dx < cellW[i]; ++dx){
				const int x0 = dx * sw / cellW[i], x1 = max(x0 + 1, (dx + 1) * sw / cellW[i]);
				int on = 0, n = 0;
				for(int sy = y0; sy < y1 && sy < sh; ++sy)
					for(int sx = x0; sx < x1 && sx < sw; ++sx, ++n)
						on += master.pixel(b, sx, sy);
				if(n)
					buf[(y + dy) * pitch + (x + dx)] = uint8(on * 255 / n);
			}
		}

		one.u = x;
		one.v = y;
		one.du = uint8(cellW[i]);
		one.dv = uint8(min<int>(pixelSize, 255));
		one.su = 0;
		one.sv = 0;
		one.lh = one.rh = 0;
		// The original stepped the pen by round(width * mul + 2): the two pixels are the
		// gap between glyphs, and they are in scaled space, so the gap stays 2px at any
		// size.  du is the texture cell, advance is the step, hence the +2 here and not
		// in du.
		one.advance = uint8(min<int>(cellW[i] + 2, 255));

		x += cellW[i] + GLYPH_PAD_SPACE;
	}

	font->texture_->UnlockTexture();

	// Worth a line: a bitmap face and a TrueType one are indistinguishable downstream, so
	// this is the only place that can say which was used, and "the text looks wrong" is
	// otherwise a hard thing to attribute.
	dprintf("BitmapFont: %s at %d px, %dx%d atlas, master %d px\n",
	        path, int(pixelSize), int(size.x), int(size.y), master.height);
	return font;
}

} // namespace FT

#endif // MAELSTROM_DATA
