#include "StdAfxRD.h"
#include "DDSImage.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Minimal DDS decoder: DXT1/DXT3/DXT5 (BCn) + uncompressed 32/24-bit, base mip
// only, output BGRA (matching cFileImage::GetTexture's byte order). See header.
// ---------------------------------------------------------------------------

namespace {

inline uint32_t rd32(const uint8_t* p) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint32_t)p[3]<<24); }
inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1]<<8)); }

inline void rgb565(uint16_t c, int& r, int& g, int& b)
{
	r = (c >> 11) & 0x1F; r = (r << 3) | (r >> 2);
	g = (c >> 5)  & 0x3F; g = (g << 2) | (g >> 4);
	b =  c        & 0x1F; b = (b << 3) | (b >> 2);
}

// Decode one BC color sub-block (8 bytes) -> 16 RGB triplets + per-texel alpha
// flag (only meaningful for DXT1's 1-bit alpha; dxt1Alpha=false for DXT3/5).
void decodeColor(const uint8_t* blk, int r[16], int g[16], int b[16], int a[16], bool dxt1Alpha)
{
	uint16_t c0 = rd16(blk), c1 = rd16(blk + 2);
	int r0,g0,b0,r1,g1,b1;
	rgb565(c0, r0,g0,b0);
	rgb565(c1, r1,g1,b1);
	int pr[4], pg[4], pb[4], pa[4];
	pr[0]=r0; pg[0]=g0; pb[0]=b0; pa[0]=255;
	pr[1]=r1; pg[1]=g1; pb[1]=b1; pa[1]=255;
	if(c0 > c1 || !dxt1Alpha){
		pr[2]=(2*r0+r1)/3; pg[2]=(2*g0+g1)/3; pb[2]=(2*b0+b1)/3; pa[2]=255;
		pr[3]=(r0+2*r1)/3; pg[3]=(g0+2*g1)/3; pb[3]=(b0+2*b1)/3; pa[3]=255;
	} else {
		pr[2]=(r0+r1)/2; pg[2]=(g0+g1)/2; pb[2]=(b0+b1)/2; pa[2]=255;
		pr[3]=0; pg[3]=0; pb[3]=0; pa[3]=0;  // transparent black
	}
	uint32_t bits = rd32(blk + 4);
	for(int i = 0; i < 16; ++i){
		int idx = (bits >> (2*i)) & 3;
		r[i]=pr[idx]; g[i]=pg[idx]; b[i]=pb[idx]; a[i]=pa[idx];
	}
}

} // namespace

int cDDSImage::load(const char* fname)
{
	int size = 0;
	char* buf = 0;
	if(!RenderFileRead(fname, buf, size))
		return 1;
	int r = load(buf, size);
	delete[] buf;
	return r;
}

int cDDSImage::load(void* pointer, int size)
{
	const uint8_t* p = (const uint8_t*)pointer;
	if(size < 128 || rd32(p) != 0x20534444 /* 'DDS ' */)
		return 1;

	const int h = (int)rd32(p + 12);   // dwHeight
	const int w = (int)rd32(p + 16);   // dwWidth
	const uint32_t pfFlags = rd32(p + 80);
	const uint32_t fourCC  = rd32(p + 84);
	if(w <= 0 || h <= 0 || w > 8192 || h > 8192)
		return 1;

	x = w; y = h; bpp = 32; length = 1; time = 0;
	bgra_.assign((size_t)w * h * 4, 0);

	const uint8_t* data = p + 128;
	const uint8_t* end  = p + size;

	auto put = [&](int px, int py, int rr, int gg, int bb, int aa){
		if(px < 0 || py < 0 || px >= w || py >= h) return;
		uint8_t* d = &bgra_[((size_t)py * w + px) * 4];
		d[0]=(uint8_t)bb; d[1]=(uint8_t)gg; d[2]=(uint8_t)rr; d[3]=(uint8_t)aa;
	};

	const bool isDXT1 = (fourCC == 0x31545844); // 'DXT1'
	const bool isDXT3 = (fourCC == 0x33545844); // 'DXT3'
	const bool isDXT5 = (fourCC == 0x35545844); // 'DXT5'

	if(isDXT1 || isDXT3 || isDXT5){
		const int blockBytes = isDXT1 ? 8 : 16;
		const int bw = (w + 3) / 4, bh = (h + 3) / 4;
		const uint8_t* src = data;
		for(int by = 0; by < bh; ++by){
			for(int bx = 0; bx < bw; ++bx){
				if(src + blockBytes > end) return 1;
				int r[16], g[16], b[16], a[16];
				const uint8_t* colorBlk = isDXT1 ? src : src + 8;
				decodeColor(colorBlk, r, g, b, a, isDXT1);
				if(isDXT3){
					// 16 explicit 4-bit alphas.
					for(int i = 0; i < 16; ++i){
						int nib = (src[i/2] >> ((i&1) ? 4 : 0)) & 0xF;
						a[i] = nib * 17;
					}
				} else if(isDXT5){
					int a0 = src[0], a1 = src[1];
					int al[8];
					al[0]=a0; al[1]=a1;
					if(a0 > a1){
						for(int i = 1; i <= 6; ++i) al[i+1] = ((7-i)*a0 + i*a1)/7;
					} else {
						for(int i = 1; i <= 4; ++i) al[i+1] = ((5-i)*a0 + i*a1)/5;
						al[6]=0; al[7]=255;
					}
					uint64_t bits = 0;
					for(int i = 0; i < 6; ++i) bits |= (uint64_t)src[2+i] << (8*i);
					for(int i = 0; i < 16; ++i)
						a[i] = al[(bits >> (3*i)) & 7];
				}
				for(int i = 0; i < 16; ++i)
					put(bx*4 + (i & 3), by*4 + (i >> 2), r[i], g[i], b[i], a[i]);
				src += blockBytes;
			}
		}
		return 0;
	}

	// Uncompressed RGB(A): use the channel masks from the pixel format.
	if(pfFlags & 0x40 /* DDPF_RGB */){
		const int rgbBits = (int)rd32(p + 88);
		const uint32_t rMask = rd32(p + 92), gMask = rd32(p + 96);
		const uint32_t bMask = rd32(p + 100), aMask = rd32(p + 104);
		const int bytespp = rgbBits / 8;
		if(bytespp < 3 || bytespp > 4) return 1;
		auto shiftOf = [](uint32_t m){ int s=0; if(!m) return 0; while(!(m&1)){ m>>=1; ++s; } return s; };
		const int rs = shiftOf(rMask), gs = shiftOf(gMask), bs = shiftOf(bMask), as = shiftOf(aMask);
		const uint8_t* src = data;
		for(int py = 0; py < h; ++py){
			for(int px = 0; px < w; ++px){
				if(src + bytespp > end) return 1;
				uint32_t v = 0;
				for(int k = 0; k < bytespp; ++k) v |= (uint32_t)src[k] << (8*k);
				int rr = rMask ? ((v & rMask) >> rs) : 0;
				int gg = gMask ? ((v & gMask) >> gs) : 0;
				int bb = bMask ? ((v & bMask) >> bs) : 0;
				int aa = aMask ? ((v & aMask) >> as) : 255;
				put(px, py, rr, gg, bb, aa);
				src += bytespp;
			}
		}
		return 0;
	}

	return 1;  // unsupported format
}

int cDDSImage::GetTexture(void* pointer, int time, int xSize, int ySize)
{
	if(bgra_.empty()) return -1;
	// DDS is top-down; copy directly (no vertical flip). Clamp to the smaller dims.
	const int cw = xSize < GetX() ? xSize : GetX();
	const int ch = ySize < GetY() ? ySize : GetY();
	uint8_t* dst = (uint8_t*)pointer;
	for(int row = 0; row < ch; ++row)
		std::memcpy(dst + (size_t)row * xSize * 4,
		            bgra_.data() + (size_t)row * GetX() * 4,
		            (size_t)cw * 4);
	return 0;
}
