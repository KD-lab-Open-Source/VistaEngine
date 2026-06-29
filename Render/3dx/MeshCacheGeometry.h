#ifndef __MESH_CACHE_GEOMETRY_H__
#define __MESH_CACHE_GEOMETRY_H__

// Cross-platform reader for the baked .3dxGB geometry cache (the LodsCache
// InPlace image). The original engine ships 3D models only as the 32-bit
// InPlaceArchive mesh cache (raw memory image, unportable to 64-bit), so off
// Windows we parse the simpler LodsCache sub-cache by hand to recover the raw
// vertex/index buffers. See Util/Serialization/InPlaceArchive.cpp for the
// on-disk format and Render/3dx/Static3dx.h for the LodCache/MemoryBlock layout.

#include <vector>
#include <cstdint>
#include <string>

namespace MeshCacheGeometry {

struct Lod
{
	int polygonNumber = 0;   // triangle count (index count / 3)
	int vertexNumber  = 0;
	int vertexSize    = 0;   // vertex stride in bytes (e.g. 36 == sVertexXYZINT1)
	std::vector<uint8_t> indexData;   // polygonNumber*3 uint16 indices
	std::vector<uint8_t> vertexData;  // vertexNumber*vertexSize bytes
	bool empty() const { return polygonNumber <= 0 || vertexNumber <= 0; }
};

// One material range within lods[0]'s index buffer, recovered from the .3dxG
// object cache (the full cStatic3dx StaticLod bunches + materials). Lets us draw
// the merged geometry as per-material textured sub-draws.
struct SubMesh
{
	int firstIndex = 0;   // offset into lods[0].indexData, in indices (offset_polygon*3)
	int indexCount = 0;   // num_polygon*3
	int material   = 0;   // imaterial
	std::string texture;  // fixed diffuse texture path for GetElement3D ("" if none)
};

struct Geometry
{
	std::vector<Lod> lods;   // 1 or 3 LODs
	Lod debris;
	std::vector<SubMesh> submeshes;  // material ranges for lods[0] (empty if no .3dxG)
};

// Maps a model file name (e.g. "Resource\\Models\\menu.3DX") to its baked
// geometry-cache path ("cacheData\\Models\\resource_models_menu.3dxGB"),
// mirroring cLib3dx::cacheName + the 'B' (buffer) suffix.
std::string cacheGeometryPath(const char* modelFileName);

// Same, without the trailing 'B': the full-object cache ("...menu.3dxG").
std::string objectCachePath(const char* modelFileName);

// Reads a ".3dxGB" file (e.g. CacheData\Models\resource_models_menu.3dxGB).
// Returns false if the file can't be opened or fails sanity checks.
bool read(const char* path, Geometry& out);

// Convenience: reads the .3dxGB geometry, then (best-effort) the .3dxG object
// cache for per-material submesh ranges + diffuse texture names.
bool readForModel(const char* modelFileName, Geometry& out);

}

#endif // __MESH_CACHE_GEOMETRY_H__
