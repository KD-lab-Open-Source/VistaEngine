#include "StdAfxRD.h"
#include "MeshCacheGeometry.h"
#include "Util/FileUtils/FileUtils.h"   // cutPathToResource

#include <cstring>

namespace MeshCacheGeometry {

// ---------------------------------------------------------------------------
// On-disk format (Util/Serialization/InPlaceArchive.cpp):
//   [int version][int size][size bytes: 32-bit memory image]
//   [int fixUpCount][fixUpCount ints][int vtableCount][...]
// Pointer/vector/MemoryBlock fields inside the image store a 4-byte OFFSET into
// the image (the loader would relocate them by +base; we read offsets directly).
//
// 32-bit struct layout (see Render/3dx/Static3dx.h, Serialization.h MemoryBlock):
//   MemoryBlock = { char* buffer_(4); int size_(4); bool makeFree_(1)+pad } = 12
//   LodCache    = { int poly; int vert; int vsize;
//                   MemoryBlock ib; MemoryBlock vb } = 12 + 12 + 12 = 36
//   LodsCache   = { vector<LodCache> lods(12); LodCache debris(36) } = 48
//   vector<T>   = { T* begin(4); T* end(4); T* cap(4) } = 12  (count=(end-begin)/sizeof T)
// ---------------------------------------------------------------------------

static const int SZ_MEMBLOCK = 12;
static const int SZ_LODCACHE  = 36;

static int32_t rd32(const std::vector<uint8_t>& b, size_t off)
{
	int32_t v = 0;
	if(off + 4 <= b.size())
		memcpy(&v, b.data() + off, 4);
	return v;
}

static float rdf32(const std::vector<uint8_t>& b, size_t off)
{
	float v = 0.f;
	if(off + 4 <= b.size())
		memcpy(&v, b.data() + off, 4);
	return v;
}

static bool readBlock(const std::vector<uint8_t>& blob, size_t structOff,
                      std::vector<uint8_t>& out)
{
	const int32_t dataOff  = rd32(blob, structOff + 0);
	const int32_t dataSize = rd32(blob, structOff + 4);
	if(dataSize <= 0)
		return true; // empty block is valid
	if(dataOff < 0 || (size_t)dataOff + (size_t)dataSize > blob.size())
		return false;
	out.assign(blob.begin() + dataOff, blob.begin() + dataOff + dataSize);
	return true;
}

static bool parseLod(const std::vector<uint8_t>& blob, size_t off, Lod& lod)
{
	lod.polygonNumber = rd32(blob, off + 0);
	lod.vertexNumber  = rd32(blob, off + 4);
	lod.vertexSize    = rd32(blob, off + 8);

	if(!readBlock(blob, off + 12, lod.indexData))   // ibBlock
		return false;
	if(!readBlock(blob, off + 24, lod.vertexData))  // vbBlock
		return false;

	// Sanity: the raw blocks must match the declared counts/stride.
	if(lod.polygonNumber > 0 && (int)lod.indexData.size() != lod.polygonNumber * 6)
		return false;
	if(lod.vertexNumber > 0 && lod.vertexSize > 0 &&
	   (int)lod.vertexData.size() != lod.vertexNumber * lod.vertexSize)
		return false;
	return true;
}

std::string cacheGeometryPath(const char* modelFileName)
{
	if(!modelFileName || !*modelFileName)
		return std::string();
	// cLib3dx::cacheName: cutPathToResource + "G" + ('\\'->'_'), prefixed by the
	// model cache dir; geometry adds a trailing 'B' (see saveInPlace).
	std::string name = cutPathToResource(modelFileName);
	name += "G";
	for(char& c : name)
		if(c == '\\' || c == '/')
			c = '_';
	return std::string("cacheData\\Models\\") + name + "B";
}

std::string objectCachePath(const char* modelFileName)
{
	std::string g = cacheGeometryPath(modelFileName);  // "...3dxGB"
	if(!g.empty() && (g.back() == 'B' || g.back() == 'b'))
		g.pop_back();                                  // -> "...3dxG"
	return g;
}

// ---------------------------------------------------------------------------
// .3dxG object cache (full cStatic3dx InPlace image) — recover, for lods[0],
// the per-material index ranges and diffuse texture names. Offsets below are
// fixed offsetof()s in the 32-bit cStatic3dx layout, verified stable across the
// shipped model caches (see Static3dx.h / Static3dxBase.h):
//   materials vector header @ image offset 100; StaticMaterial stride 252,
//     name string @ +0, tex_diffuse string @ +72.
//   lods vector header @ image offset 296; StaticLod stride 32,
//     bunches vector @ +12; StaticBunch stride 44 = {offset_polygon, num_polygon,
//     offset_vertex, num_vertex, imaterial} (5 ints) + 2 vector headers.
// String header = {begin(off), end(off=null pos), cap(off)}; text = [begin,end).
// ---------------------------------------------------------------------------
static const int OBJ_MATERIALS_HDR = 100;
static const int OBJ_LODS_HDR      = 296;
static const int SZ_MATERIAL       = 252;
static const int MAT_DIFFUSE       = 28;  // diffuse Color4f (rgba floats)
static const int MAT_OPACITY       = 60;  // opacity float
static const int MAT_TEXDIFFUSE    = 72;
static const int MAT_TRANSPARENCY  = 88;  // transparencyType enum int
static const int SZ_STATICLOD      = 32;
static const int LOD_BUNCHES       = 12;
static const int SZ_BUNCH          = 44;

// Read the InPlace string at the given header offset (begin/end offsets).
static std::string readImageString(const std::vector<uint8_t>& blob, size_t hdrOff)
{
	const int32_t begin = rd32(blob, hdrOff + 0);
	const int32_t end   = rd32(blob, hdrOff + 4);
	if(begin <= 0 || end < begin || (size_t)end > blob.size() || (end - begin) > 1024)
		return std::string();
	return std::string(reinterpret_cast<const char*>(blob.data() + begin), (size_t)(end - begin));
}

// extractFilePath(modelFileName): keep through the last path separator.
static std::string filePath(const char* name)
{
	std::string s = name ? name : "";
	size_t p = s.find_last_of("\\/");
	return p == std::string::npos ? std::string() : s.substr(0, p + 1);
}
// extractFileName(name): drop any directory part.
static std::string fileLeaf(const std::string& name)
{
	size_t p = name.find_last_of("\\/");
	return p == std::string::npos ? name : name.substr(p + 1);
}

static void parseObjectMaterials(const std::vector<uint8_t>& blob, const char* modelFileName,
                                 const Lod& lod0, std::vector<SubMesh>& out)
{
	out.clear();
	if(lod0.polygonNumber <= 0)
		return;

	// materials: gather diffuse texture name per material index.
	const int32_t mBegin = rd32(blob, OBJ_MATERIALS_HDR + 0);
	const int32_t mEnd   = rd32(blob, OBJ_MATERIALS_HDR + 4);
	if(mBegin <= 0 || mEnd < mBegin || (mEnd - mBegin) % SZ_MATERIAL != 0)
		return;
	const int numMat = (mEnd - mBegin) / SZ_MATERIAL;
	if(numMat <= 0 || numMat > 4096)
		return;

	// fixTextureName: extractFilePath(model) + "Textures\\" + extractFileName(tex).
	const std::string texDir = filePath(modelFileName) + "Textures\\";
	std::vector<std::string> matTex((size_t)numMat);
	struct MatProps { float diffuse[4]; float opacity; int transparency; };
	std::vector<MatProps> matProps((size_t)numMat);
	for(int i = 0; i < numMat; ++i){
		const size_t base = (size_t)mBegin + (size_t)i * SZ_MATERIAL;
		std::string diffuse = readImageString(blob, base + MAT_TEXDIFFUSE);
		if(!diffuse.empty())
			matTex[i] = texDir + fileLeaf(diffuse);
		MatProps& mp = matProps[(size_t)i];
		for(int c = 0; c < 4; ++c)
			mp.diffuse[c] = rdf32(blob, base + MAT_DIFFUSE + (size_t)c * 4);
		mp.opacity = rdf32(blob, base + MAT_OPACITY);
		mp.transparency = rd32(blob, base + MAT_TRANSPARENCY);
	}

	// lods[0] bunches: each maps a polygon range to a material.
	const int32_t lBegin = rd32(blob, OBJ_LODS_HDR + 0);
	const int32_t lEnd   = rd32(blob, OBJ_LODS_HDR + 4);
	if(lBegin <= 0 || lEnd < lBegin || (lEnd - lBegin) % SZ_STATICLOD != 0 || lEnd == lBegin)
		return;
	const size_t lod0Off = (size_t)lBegin;  // StaticLod[0]
	const int32_t bBegin = rd32(blob, lod0Off + LOD_BUNCHES + 0);
	const int32_t bEnd   = rd32(blob, lod0Off + LOD_BUNCHES + 4);
	if(bBegin <= 0 || bEnd < bBegin || (bEnd - bBegin) % SZ_BUNCH != 0)
		return;
	const int numBunch = (bEnd - bBegin) / SZ_BUNCH;
	if(numBunch <= 0 || numBunch > 65536)
		return;

	int polyTotal = 0;
	std::vector<SubMesh> subs;
	subs.reserve((size_t)numBunch);
	for(int i = 0; i < numBunch; ++i){
		const size_t o = (size_t)bBegin + (size_t)i * SZ_BUNCH;
		const int op = rd32(blob, o + 0);
		const int np = rd32(blob, o + 4);
		const int im = rd32(blob, o + 16);
		if(op < 0 || np <= 0 || op + np > lod0.polygonNumber || im < 0 || im >= numMat)
			return;  // layout mismatch — abort, leave geometry untextured
		polyTotal += np;
		SubMesh s;
		s.firstIndex = op * 3;
		s.indexCount = np * 3;
		s.material   = im;
		s.texture    = matTex[(size_t)im];
		const MatProps& mp = matProps[(size_t)im];
		for(int c = 0; c < 4; ++c)
			s.diffuse[c] = mp.diffuse[c];
		s.opacity      = mp.opacity;
		s.transparency = mp.transparency;
		subs.push_back(s);
	}
	// Must exactly partition the LOD's triangles to trust the parse.
	if(polyTotal != lod0.polygonNumber)
		return;

	out.swap(subs);
}

bool readForModel(const char* modelFileName, Geometry& out)
{
	std::string path = cacheGeometryPath(modelFileName);
	if(path.empty() || !read(path.c_str(), out))
		return false;

	// Best-effort: layer the .3dxG material/submesh ranges over lods[0].
	out.submeshes.clear();
	std::string objPath = objectCachePath(modelFileName);
	if(!objPath.empty() && !out.lods.empty()){
		XStream ff(0);
		if(ff.open(objPath.c_str(), XS_IN)){
			int32_t version = 0, size = 0;
			ff.read(version);
			ff.read(size);
			if(size > 0 && size <= (1 << 28)){
				std::vector<uint8_t> blob((size_t)size);
				if(ff.read(blob.data(), size) == (unsigned long)size)
					parseObjectMaterials(blob, modelFileName, out.lods[0], out.submeshes);
			}
		}
	}
	return true;
}

bool read(const char* path, Geometry& out)
{
	out.lods.clear();
	out.debris = Lod();

	XStream ff(0);
	if(!ff.open(path, XS_IN))
		return false;

	int32_t version = 0, size = 0;
	ff.read(version);
	ff.read(size);
	if(size <= 0 || size > (1 << 28))
		return false;

	std::vector<uint8_t> blob((size_t)size);
	if(ff.read(blob.data(), size) != (unsigned long)size)
		return false;

	// LodsCache @ 0: vector<LodCache> lods at offset 0, LodCache debris at 12.
	const int32_t beginOff = rd32(blob, 0);
	const int32_t endOff   = rd32(blob, 4);
	if(beginOff < 0 || endOff < beginOff || (endOff - beginOff) % SZ_LODCACHE != 0)
		return false;

	const int count = (endOff - beginOff) / SZ_LODCACHE;
	if(count < 0 || count > 4096)
		return false;

	out.lods.resize(count);
	for(int i = 0; i < count; ++i)
		if(!parseLod(blob, (size_t)beginOff + (size_t)i * SZ_LODCACHE, out.lods[i]))
			return false;

	parseLod(blob, SZ_MEMBLOCK /*=12, debris offset*/, out.debris);

	return !out.lods.empty();
}

}
