# InPlace Archive — format, mechanism, and the `.3dxG` model cache

Reference for how `InPlaceOArchive` (`Util/Serialization/InPlaceArchive.{h,cpp}`)
serializes a live C++ object graph to disk, why it round-trips across different
process memory layouts, and how to decode a `.3dxG` file (a saved `cStatic3dx`)
without the engine. A standalone decoder lives at **`tools/read_3dxG.py`**.

- Model caches: `GameData/CacheData/Models/*.3dxG` = a full `cStatic3dx` object;
  `*.3dxGB` = the companion `LodsCache` (raw VB/IB geometry).
- Written by `cStatic3dx::saveInPlace()` (`Render/3dx/Static3dx.cpp`).

---

## 1. What "InPlace" means

`InPlaceOArchive` does **not** emit a tagged/keyed stream. It writes a **raw
32-bit memory image** of the object graph plus a relocation table. Loading is
almost free: read the blob, patch a list of offsets into pointers, and
`reinterpret_cast` the blob to the object (`InPlaceIArchive::construct`).

The root dump happens in `openStructInternal` (InPlaceArchive.cpp:73). For the
first (root) struct it does:

```cpp
stack_.push_back(Node(object, size, saver_.size()));
saver_.write(object, size);          // memcpy the ENTIRE object (bases + padding)
```

So the image starts as a byte-for-byte copy of the object, and every nested
*value* member is inline at its natural C++ offset. Members that hold pointers
(`std::string`, `std::vector`, `MemoryBlock`, raw `T*`) get their pointer words
rewritten to **image-relative offsets**, and the pointed-to bytes are appended
after the root. See §4.

---

## 2. On-disk file layout (`InPlaceOArchive::close`, InPlaceArchive.cpp:25)

```
int32   version                              // 0 for the shipped caches
int32   imageSize
byte    image[imageSize]                      // the 32-bit object dump (root + appended data)
int32   fixupCount
int32   fixupOffsets[fixupCount]              // image slots holding a pointer-offset to relocate
int32   vtableFixupCount
struct { int32 offset; char typeName[]; }     // one per polymorphic subobject
        vtableFixups[vtableFixupCount]         //   typeName is NUL-terminated
```

`resource_models_e_meetpoint.3dxG`: `version=0, imageSize=6649, fixupCount=169,
vtableFixupCount=1 -> {offset 0, "class cStatic3dx"}`.

Everything after `image` is only needed to relocate a *live* object. A pure
reader interprets the stored offsets as image indices and never relocates.

---

## 3. The dispatch that visits every field (`Serialization.h`)

The generic entry point routes each type to a handler **at compile time**:

```cpp
Select< IsClass<T>,
    Select< HaveAdvancedSerialization<T>,
        Identity< ProcessAdvancedNonPrimitiveImpl<T> >,   // t.serialize(ar, name, nameAlt)
        Identity< ProcessNonPrimitiveImpl<T> >            // openStruct; t.serialize(ar); closeStruct
    >,
    Identity< ProcessEnumImpl<T> >                        // enum written as int
>::type::invoke(*this, const_cast<T&>(t), name, nameAlt);
```

Read as a type-level if/else:

```
if T is a class:
    if T has bool serialize(Archive&, const char*, const char*):  ProcessAdvancedNonPrimitiveImpl<T>
    else (plain void serialize(Archive&)):                        ProcessNonPrimitiveImpl<T>
else (T is an enum):                                             ProcessEnumImpl<T>
```

Pieces:
- **`IsClass<T>`** — SFINAE trait: a pointer-to-member `void(U::*)()` is only
  formable for a class type, so it detects class-vs-enum. (Fundamental types and
  the special types — `vector`, `string`, `pair`, `MemoryBlock`, POD scalars,
  `BitVector` — never reach here; they have dedicated `serialize` overloads.)
- **`HaveAdvancedSerialization<T>`** — overload-resolution trait on
  `&T::serialize`: distinguishes the plain member `void serialize(Archive&)`
  (normal UDTs) from the advanced `bool serialize(Archive&, const char*, const char*)`
  used by wrapper types (`ShareHandle`, `PolymorphicWrapper`, `PointerWrapper`,
  `UniqueVector`, `Interpolator3dx`, `SplineData`).
- **`Select<C,T1,T2>`** — a meta-ternary (`Selector<C::value,…>` picks T1/T2,
  then unwraps `::type`).
- **`Identity<X>`** — `typedef X type;`. The laziness lever: `Select` first
  *chooses* a branch, then asks for `::type`, so the **non-chosen** handler is
  never instantiated (avoids compiling `ProcessEnumImpl<SomeClass>` etc.).

Handlers:
- `ProcessNonPrimitiveImpl<T>::invoke` → `if(openStruct(t)){ t.serialize(ar); closeStruct(); }`.
  **This is the path that visits every property**: the type's hand-written
  `serialize(Archive&)` calls `ar.serialize(field, …)` per member, each of which
  recurses through this same dispatch.
- `ProcessAdvancedNonPrimitiveImpl<T>::invoke` → `t.serialize(ar, name, nameAlt)`
  (wrapper opens/closes the pointer/container itself).
- `ProcessEnumImpl<T>::invoke` → read/write the enum as an `int`.

The Python reader mirrors this recursion, so it reads everything the writer
wrote.

---

## 4. Pointers across processes: offset swizzling

A raw pointer is only valid in the process that produced it — every run/OS maps
the heap elsewhere. So the archive stores **offsets** and relocates them on load
(classic pointer swizzling, like ELF/PE base relocations).

### 4a. address → offset (`Node`)

`InPlaceArchive.h` keeps a stack of `Node(address, size, offset)`. For any field
inside a tracked struct:

```cpp
int offset(const char* addr) const { return addr - address_ + offset_; }
// field_offset = (field_addr - struct_addr) + struct_image_offset
```

Because the struct was copied into the image preserving its internal layout, the
field's relative position is the same in memory and in the image.

### 4b. `std::string` — 3 pointers → 3 offsets + appended text (`writeString`, :105)

STLport `string` is a 12-byte header `{begin, end, end_of_storage}`.

```cpp
int offset = stack_.back().offset(&str);
fixUpSaver_.write(offset);  fixUpSaver_.write(offset+4);  fixUpSaver_.write(offset+8);
int dataOffset = saver_.size();
saver_.write(str.c_str());          // strlen+1 bytes (incl NUL)
int dataEnd = saver_.size();
saver_.set(offset);
saver_.write(dataOffset);            // begin = start of text
saver_.write(dataEnd - 1);           // end   = the NUL   (length = end - begin)
saver_.write(dataEnd);               // cap   = one past
```

Concrete (`fileName_` header at image offset 248):

```
begin=420 end=451 cap=452  ->  image[420:451] = "RESOURCE\MODELS\E_MEETPOINT.3DX"  (len 31)
```

At load `420/451/452 += base` → three valid `char*` **into the blob**.

### 4c. `std::vector<T>` — header + raw element copy, recursive (`openContainer`, :128)

`vector` is also `{begin, end, end_of_storage}`.

```cpp
int offset = stack_.back().offset(array);
char* data = *(char**)array;                     // live heap buffer
int dataOffset = saver_.size();
saver_.write(data, number*elementSize);          // copy raw element bytes
saver_.write(dataOffset);                        // begin
saver_.write(dataEnd);                           // end  = begin + count*elemSize
saver_.write(dataEnd);                           // cap
stack_.push_back(Node(data, size, dataOffset));  // element fields now map addr->offset
```

`count = (end - begin) / sizeof(T)`. Examples:

```
materials header@100: begin=1887 end=2139 -> (2139-1887)/252 = 1 element
nodes     header@52 : begin=555  end=795  -> (795-555)/80    = 3 elements
```

- **POD elements** (vertex arrays, `vector<sPolygon>`) are just the raw copy —
  no per-element fixups, only the 3 header slots.
- **Elements with pointers** are handled by recursion: after `openContainer` the
  template loops `serialize(*it)`, so each element's strings/vectors rewrite
  their own offsets and append their own data. A `vector<string>` becomes:
  header (3 offsets) → array of N string-headers (3 offsets each) → N text blobs.
  e.g. a visibility group's `meshes` at header@1795: begin=1819 end=1855 → 3 strings.

### 4d. `T*` — one offset to an appended pointee (`openPointer`, :175)

```cpp
int offset = stack_.back().offset(&object);
fixUpSaver_.write(offset);
int dataOffset = saver_.size();
saver_.set(offset); saver_.write(dataOffset);    // pointer field now holds an offset
saver_.set(dataOffset);
beginBlock_ = true;                              // next openStructInternal dumps the pointee here
```

On load the slot `+= base`. (In `cStatic3dx` the only serialized pointer is
`debrises`, cast to `vector<int>` and asserted empty; texture pointers like
`pBumpTexture` are deliberately not serialized.)

### 4e. The vtable pointer — can't be an offset

A vtable lives in the code segment, so it differs per build/load. In
`openStructInternal` (:86) the vptr is zeroed and recorded as `{offset, typeName}`.
On load `VTableFactory::getVTable(typeName)` resolves the *current* process's
vtable:

```cpp
*(int*)(data_ + offset) = (int)(intptr_t)VTableFactory::getVTable(typeName);
```

### 4f. The load side (`InPlaceIArchive::open`, :211)

```cpp
while(fixUpSize--)
    *(int*)(data_ + *fixUp++) += (int)(intptr_t)data_;   // every offset -> real pointer
// then the vtable fixups (above)
```

After this, every `string`/`vector`/`T*` in the blob points **into the same
blob**, and vptrs are valid → the blob is a usable object with no per-field work.

---

## 5. Consequences

- **Containers alias the blob** — they do not own their buffers. So you must
  never run their normal destructors. `cStatic3dx::Release()` on an in-place
  object skips `~cStatic3dx()` and calls `InPlaceIArchive::destruct(this)` which
  just frees the single blob. Running `~string`/`~vector` would `free()` an
  interior pointer → crash.
- **Not 64-bit portable** — offsets are 32-bit `int`, layout uses 32-bit pointer
  sizes + MSVC/STLport padding, and relocation truncates under a 64-bit process.
  The format round-trips only across runs of the original 32-bit engine.
  Off-Windows we therefore decode the offsets by hand (`tools/read_3dxG.py`,
  `Render/3dx/MeshCacheGeometry.cpp`).
- **Only `serialize()`-visited fields are valid.** A field the method skips still
  sits in the raw dump with a *stale live pointer* (e.g. `StaticVisibilitySet::meshes`,
  all `pTexture`/`pBump…` pointers). Reading such a header as a vector yields
  garbage — the decoder must skip them. Non-visited **POD** fields (e.g.
  `voxelBox.valid_/size_`, `cTempVisibleGroup::visibilityNodeIndex`) are still
  inline-readable.

---

## 6. Confirmed 32-bit sizes (the build that produced the caches)

```
std::string = std::vector = MemoryBlock = 12   (all {begin,end,cap}-style; count=(end-begin)/sizeof)
Vect2f 8   Vect3f 12   Color4f 16   Color4c 4   Mat3f 36   MatXf 48   sBox6f 24
sRectangle4f 16   sPolygon 6   bool 1   int/float/DWORD/enum/ptr 4
UnknownClass 8            (vptr @0, long m_cRef @4)
Interpolator3dx<T> = 12   (just a vector<T>)
SplineData<n> = 12 + n*16 (Scale<1>=28, Position<3>=60, Rotation<4>=76, UV<6>=108)
SplineDataBool = 12
```

---

## 7. `cStatic3dx` image offset table

Root layout: `cStatic3dx` = `UnknownClass`(8) + `Static3dxBase`(288, starts @8) +
own members (@296). Cross-checked against the file: `materials@100`, `lods@296`,
material stride 252, `StaticLod` 32, `StaticBunch` 44.

`Static3dxBase` subobject (image offsets):

```
version@8   maxWeights@12
nonDeleteNodes@16   logicNodes@28   boundNodes@40                 (vector<string>)
nodes@52 (StaticNode, stride 80)
animationGroups_@64 (AnimationGroup, 48)
animationChains_@76 (StaticAnimationChain, 28)
visibilitySets_@88 (StaticVisibilitySet, 36)
materials@100 (StaticMaterial, 252)
isBoundBoxInited@112   boundBox@116 (sBox6f)   boundRadius@140
localLogicBounds@144 (sBox6f, 24)
logos@156 (sLogo, 36)   effects@168 (StaticEffect, 20)
lights@180 (StaticLight, 56)   leaves@192 (StaticLeaf, 52)
is_lod@204 is_logic@205 is_old_model@206 loaded@207
circle_shadow_enable@208 _min@212 height@216 radius@220
boundSpheres@224 (BoundSphere, 20)
bump@236 isUV2@237 enableFur@238
cameraParams@240 (camera_node_num@240, fov@244)
fileName_@248 (string; only written when inPlace())
tempMesh_@260  tempMeshLod1_@272  tempMeshLod2_@284   (vector<ShareHandle>, usually empty)
```

`cStatic3dx` own members:

```
lods@296 (StaticLod, stride 32)   debris@308 (StaticLod)   debrises@340 (vector, empty)
voxelBox@352 (64 bytes): valid_@352 sizeLen_@356 size_@360 mask_@364
                         scale_@368 scaleInv_@380 offset_@392 buffer_@404 (MemoryBlock)
inPlace_@416
```

Sub-struct layouts:

```
StaticNode (80):   name@0(string) inode@12 iparent@16 chains@20(vec<StaticNodeAnimation>,48) inv_begin_pos@32(MatXf)
StaticNodeAnimation (48): scale@0 position@12 rotation@24 visibility@36   (each Interpolator3dx = vector)
AnimationGroup (48): name@0 nodes@12(vec<int>) nodesNames@24(vec<string>) materialsNames@36(vec<string>)
StaticAnimationChain (28): name@0 time@12 begin_frame@16 end_frame@20 cycled@24
StaticVisibilitySet (36): name@0  [meshes@12 NOT serialized]  visibilityGroups@24(vec<...>,44)
StaticVisibilityGroup (44): name@0 visibility@12 visibleNodes@16(vec<char>) meshes@28(vec<string>) is_invisible_list@40
StaticMaterial (252): name@0 ambient@12 diffuse@28 specular@44 opacity@60 specular_power@64
    is_opacity_texture@68 tex_diffuse@72 tiling_diffuse@84 transparencyType@88 is_skinned@92
    tex_skin@96 tex_bump@108 [pBumpTexture@120] tex_reflect@124 [pReflect@136] reflect_amount@140
    is_reflect_sky@144 [pSpecularmap@148] tex_specularmap@152 tex_self_illumination@164
    [pSecondOpacity@176] tex_secondopacity@180 animation_group_index@192 no_light@196
    is_big_ambient@197 texturesCreated@198 chains@200(vec<StaticMaterialAnimation>,36)
    [pFurmap@212] fur_scale@216 fur_alpha@220 tex_furmap@224 tex_furnormalmap@236 fur_alpha_type@248
StaticMaterialAnimation (36): opacity@0(Scale) uv@12(UV) uv_displacement@24(UV)
StaticEffect (20): node@0 is_cycled@4 file_name@8(string)
StaticLight (56): inode@0 color@4(Color4f) atten_start@20 atten_end@24 chains@28(vec<StaticLightAnimation>,12) texture@40 [pTexture@52]
StaticLeaf (52): inode@0 color@4 size@20 texture@24(string) [pTexture@36] lods@40(vec<int>)
sLogo (36): rect@0(sRectangle4f) TextureName@16(string) angle@28 enabled@32
BoundSphere (20): node_index@0 position@4(Vect3f) radius@16
StaticLod (32): [ib@0][vb@4] blend_indices@8 bunches@12(vec<StaticBunch>,44) [sys_vb@24][sys_ib@28]
StaticBunch (44): offset_polygon@0 num_polygon@4 offset_vertex@8 num_vertex@12 imaterial@16
                  nodeIndices@20(vec<int>) visibleGroups@32(vec<cTempVisibleGroup>,20)
cTempVisibleGroup (20): visible_set@0 visibilities@4 begin_polygon@8 num_polygon@12 visibilityNodeIndex@16
```

`[field]` = present in the layout but not serialized (skip its header).

---

## 8. Companion `.3dxGB` (`LodsCache`)

Same InPlace format. Image root = `LodsCache { vector<LodCache> lods@0; LodCache debris@12 }`.

```
LodCache (36): polygonNumber@0 vertexNumber@4 vertexSize@8 ibBlock@12(MemoryBlock) vbBlock@24(MemoryBlock)
```

Sanity checks that also validate the `.3dxG` bunch counts:
`ibBytes == polygonNumber*6` (uint16 triangle indices),
`vbBytes == vertexNumber*vertexSize`.
Meetpoint: 1 LOD, poly 314, vert 365, vsize 36 — matches the `StaticBunch`.

---

## 9. Using the decoder

```
python3 tools/read_3dxG.py GameData/CacheData/Models/resource_models_e_meetpoint.3dxG
```

Prints the full `cStatic3dx` tree (nodes, animation splines, materials +
textures, visibility sets, bounds, lights, LOD bunches, voxel box). It reads the
stored offsets directly (no relocation) and guards every indirect header with a
bounds check, marking non-serialized/stale slots as `<unserialized>`.
