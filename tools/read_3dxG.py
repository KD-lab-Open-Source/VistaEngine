#!/usr/bin/env python3
"""
read_3dxG.py  --  Decode a Perimeter-2 ".3dxG" model cache and print the whole
                  cStatic3dx object tree.

WHY THIS WORKS
--------------
".3dxG" is written by cStatic3dx::saveInPlace() through InPlaceOArchive
(Util/Serialization/InPlaceArchive.cpp).  That archive does NOT emit a
field-tagged stream: it writes a *raw 32-bit memory image* of the object graph.

  * openStructInternal() at the root does `saver_.write(object, sizeof(object))`
    -- i.e. it memcpy's the entire cStatic3dx (all bases + padding) into the
    image.  Nested value members are therefore inline at their natural C++
    offsets.
  * POD fields (int/float/bool/enum/Vect/Color/Mat...) are left exactly where
    the raw dump put them -- processValue() only asserts, it writes nothing.
  * std::string / std::vector / MemoryBlock are 12-byte {begin,end,cap}-style
    headers whose pointer words are rewritten to hold an OFFSET into the image;
    the pointed-to bytes are appended after the root.
  * raw pointers store an offset too (openPointer); the single vtable pointer at
    offset 0 is zeroed and recorded in a side "vtable fixup" table.

On load, InPlaceIArchive::open() just relocates every recorded offset by adding
the base address (`*(int*)(data+off) += base`) and casts the image to the
object.  We do the mirror image here: we read the file, and interpret every
stored offset as an image-relative index -- so we never need to relocate.

FILE LAYOUT (InPlaceOArchive::close)
    int32  version
    int32  imageSize
    byte   image[imageSize]                     <-- the 32-bit object dump
    int32  fixupCount
    int32  fixupOffsets[fixupCount]             <-- slots to relocate (pointers)
    int32  vtableFixupCount
    { int32 offset; char typeName[]; } * vtableFixupCount

Everything below the header is only needed to relocate a live object; for
reading we walk the image using the 32-bit struct layouts recovered from the
C++ headers (Static3dx.h / Static3dxBase.h / xmath.h / ...).

Usage:  python3 read_3dxG.py path/to/model.3dxG
"""

import struct
import sys

# ---- fixed 32-bit sizes (MSVC / STLport build the cache was produced with) ---
STRING = 12      # std::string  = {begin,end,end_of_storage} offsets
VECTOR = 12      # std::vector  = {begin,end,end_of_storage} offsets
MEMBLK = 12      # MemoryBlock  = {char* buffer; int size; bool makeFree;}


class Image:
    """The raw 32-bit object image plus typed readers at absolute offsets."""

    def __init__(self, data):
        self.d = data

    # --- scalar POD ---------------------------------------------------------
    def i32(self, o):  return struct.unpack_from("<i", self.d, o)[0]
    def u32(self, o):  return struct.unpack_from("<I", self.d, o)[0]
    def i16(self, o):  return struct.unpack_from("<h", self.d, o)[0]
    def u16(self, o):  return struct.unpack_from("<H", self.d, o)[0]
    def u8(self, o):   return self.d[o]
    def f32(self, o):  return struct.unpack_from("<f", self.d, o)[0]
    def boolean(self, o): return self.d[o] != 0

    def vec3(self, o): return struct.unpack_from("<3f", self.d, o)       # 12
    def vec2(self, o): return struct.unpack_from("<2f", self.d, o)       # 8
    def col4f(self, o): return struct.unpack_from("<4f", self.d, o)      # 16
    def col4c(self, o):                                                  # 4  (b,g,r,a)
        b, g, r, a = self.d[o:o + 4]
        return (r, g, b, a)
    def matxf(self, o):                                                  # 48 (Mat3f R; Vect3f d)
        R = struct.unpack_from("<9f", self.d, o)
        dv = struct.unpack_from("<3f", self.d, o + 36)
        return R, dv

    # --- indirect (offset-carrying) members ---------------------------------
    def _valid(self, begin, end):
        # A relocated offset always lands inside the image with begin<=end.
        # Non-serialized indirect members still hold stale live pointers -> reject.
        n = len(self.d)
        return 0 <= begin <= end <= n

    def string(self, o):
        """std::string header at absolute offset `o` -> python str."""
        begin = self.i32(o)
        end = self.i32(o + 4)
        if begin == 0 and end == 0:
            return ""
        if not self._valid(begin, end) or end - begin > 4096:
            return "<stale/unserialized>"
        return self.d[begin:end].decode("cp1251", "replace")

    def vector(self, o, elem_size):
        """std::vector header at `o` -> (base_offset, count)."""
        begin = self.i32(o)
        end = self.i32(o + 4)
        if begin == 0 and end == 0:
            return begin, 0
        if elem_size <= 0 or not self._valid(begin, end):
            return begin, -1        # -1 flags an unreadable / non-serialized header
        count = (end - begin) // elem_size
        return begin, count

    def memblock(self, o):
        """MemoryBlock at `o` -> (bytes, size)."""
        buf = self.i32(o)
        size = self.i32(o + 4)
        if size <= 0:
            return b"", size
        return self.d[buf:buf + size], size


# ------------------------------- pretty printer ------------------------------
class Tree:
    def __init__(self):
        self.lines = []

    def add(self, depth, text):
        self.lines.append("  " * depth + text)

    def __str__(self):
        return "\n".join(self.lines)


def fmt_floats(t, nd=4):
    return "(" + ", ".join(f"{v:.{nd}g}" for v in t) + ")"


# ------------------------------- element readers -----------------------------
# All offsets below are RELATIVE to the element's own base; strides come from
# the 32-bit C++ layout and are asserted against the memory-cache checkpoints.

def read_spline_interp(img, t, depth, name, off, data_floats):
    """Interpolator3dx<SplineData<n>> : vector<SplineData>. SplineData size = 12 + n*16."""
    esize = 12 + data_floats * 16
    base, n = img.vector(off, esize)
    if n <= 0:
        t.add(depth, f"{name}: <empty>")
        return
    t.add(depth, f"{name}: {n} key(s)")
    ITPL = {0: "CONSTANT", 1: "LINEAR", 2: "SPLINE"}
    for i in range(n):
        b = base + i * esize
        itpl = img.i32(b)
        tbegin = img.f32(b + 4)
        inv_ts = img.f32(b + 8)
        t.add(depth + 1, f"[{i}] itpl={ITPL.get(itpl, itpl)} tbegin={tbegin:.4g} inv_tsize={inv_ts:.4g}")


def read_bool_interp(img, t, depth, name, off):
    """Interpolator3dxBool : vector<SplineDataBool>, SplineDataBool size 12."""
    base, n = img.vector(off, 12)
    if n <= 0:
        t.add(depth, f"{name}: <empty>")
        return
    t.add(depth, f"{name}: {n} key(s)")
    for i in range(n):
        b = base + i * 12
        t.add(depth + 1, f"[{i}] tbegin={img.f32(b):.4g} inv_tsize={img.f32(b+4):.4g} value={img.i32(b+8)}")


def read_string_vector(img, t, depth, name, off):
    base, n = img.vector(off, STRING)
    if n < 0:
        t.add(depth, f"{name}: <unserialized>")
        return
    vals = [img.string(base + i * STRING) for i in range(n)]
    t.add(depth, f"{name}: {vals} ({n})")


def read_int_vector(img, t, depth, name, off):
    base, n = img.vector(off, 4)
    if n < 0:
        t.add(depth, f"{name}: <unserialized>")
        return
    vals = [img.i32(base + i * 4) for i in range(n)]
    t.add(depth, f"{name}: {vals} ({n})")


def read_node(img, t, depth, i, base):
    STRIDE = 80
    t.add(depth, f"[{i}] StaticNode  name={img.string(base)!r} "
                 f"inode={img.i32(base+12)} iparent={img.i32(base+16)}")
    R, dv = img.matxf(base + 32)
    t.add(depth + 1, f"inv_begin_pos.trans={fmt_floats(dv)}")
    cbase, cn = img.vector(base + 20, 48)         # chains: vector<StaticNodeAnimation>, stride 48
    if cn:
        t.add(depth + 1, f"chains: {cn}")
        for c in range(cn):
            cb = cbase + c * 48
            t.add(depth + 2, f"[{c}] StaticNodeAnimation")
            read_spline_interp(img, t, depth + 3, "scale", cb + 0, 1)
            read_spline_interp(img, t, depth + 3, "position", cb + 12, 3)
            read_spline_interp(img, t, depth + 3, "rotation", cb + 24, 4)
            read_bool_interp(img, t, depth + 3, "visibility", cb + 36)


def read_animation_group(img, t, depth, i, base):
    t.add(depth, f"[{i}] AnimationGroup name={img.string(base)!r}")
    read_int_vector(img, t, depth + 1, "nodes", base + 12)
    read_string_vector(img, t, depth + 1, "nodesNames", base + 24)
    read_string_vector(img, t, depth + 1, "materialsNames", base + 36)


def read_animation_chain(img, t, depth, i, base):
    t.add(depth, f"[{i}] StaticAnimationChain name={img.string(base)!r} "
                 f"time={img.f32(base+12):.4g} begin_frame={img.i32(base+16)} "
                 f"end_frame={img.i32(base+20)} cycled={img.boolean(base+24)}")


def read_vis_group(img, t, depth, i, base):
    # layout: name@0 visibility@12 visibleNodes(vec<char>)@16 meshes(vec<str>)@28 is_invisible_list@40
    t.add(depth, f"[{i}] StaticVisibilityGroup name={img.string(base)!r} "
                 f"visibility=0x{img.u32(base+12):08x} is_invisible_list={img.boolean(base+40)}")
    vb, vn = img.vector(base + 16, 1)             # vector<char>
    t.add(depth + 1, f"visibleNodes: {vn} flags")
    read_string_vector(img, t, depth + 1, "meshes", base + 28)


def read_vis_set(img, t, depth, i, base):
    # NB: `meshes` (base+12) is NOT serialized -> its header holds a stale live
    # pointer, so we never touch it. Only name + visibilityGroups are written.
    t.add(depth, f"[{i}] StaticVisibilitySet name={img.string(base)!r}")
    gb, gn = img.vector(base + 24, 44)            # visibilityGroups stride 44
    t.add(depth + 1, f"visibilityGroups: {gn}")
    for g in range(gn):
        read_vis_group(img, t, depth + 2, g, gb + g * 44)


def read_mat_anim(img, t, depth, base):
    read_spline_interp(img, t, depth, "opacity", base + 0, 1)          # Scale<1>
    read_spline_interp(img, t, depth, "uv", base + 12, 6)              # UV<6>
    read_spline_interp(img, t, depth, "uv_displacement", base + 24, 6)


def read_material(img, t, depth, i, base):
    STRIDE = 252
    TRANSP = {0: "SUBSTRACTIVE", 1: "ADDITIVE", 2: "FILTER"}
    t.add(depth, f"[{i}] StaticMaterial name={img.string(base)!r}")
    t.add(depth + 1, f"ambient={fmt_floats(img.col4f(base+12))} "
                     f"diffuse={fmt_floats(img.col4f(base+28))} "
                     f"specular={fmt_floats(img.col4f(base+44))}")
    t.add(depth + 1, f"opacity={img.f32(base+60):.4g} specular_power={img.f32(base+64):.4g} "
                     f"is_opacity_texture={img.boolean(base+68)}")
    t.add(depth + 1, f"tiling_diffuse={img.i32(base+84)} "
                     f"transparencyType={TRANSP.get(img.i32(base+88), img.i32(base+88))} "
                     f"is_skinned={img.boolean(base+92)} is_reflect_sky={img.boolean(base+144)}")
    t.add(depth + 1, f"reflect_amount={img.f32(base+140):.4g} "
                     f"no_light={img.boolean(base+196)} "
                     f"animation_group_index={img.i32(base+192)}")
    for nm, o in (("tex_diffuse", 72), ("tex_skin", 96), ("tex_bump", 108),
                  ("tex_reflect", 124), ("tex_specularmap", 152),
                  ("tex_self_illumination", 164), ("tex_secondopacity", 180),
                  ("tex_furmap", 224), ("tex_furnormalmap", 236)):
        s = img.string(base + o)
        if s:
            t.add(depth + 1, f"{nm}={s!r}")
    t.add(depth + 1, f"fur_scale={img.f32(base+216):.4g} fur_alpha={img.f32(base+220):.4g} "
                     f"fur_alpha_type={img.i32(base+248)}")
    cb, cn = img.vector(base + 200, 36)           # chains: StaticMaterialAnimations, stride 36
    if cn:
        t.add(depth + 1, f"chains: {cn}")
        for c in range(cn):
            t.add(depth + 2, f"[{c}] StaticMaterialAnimation")
            read_mat_anim(img, t, depth + 3, cb + c * 36)


def read_effect(img, t, depth, i, base):
    t.add(depth, f"[{i}] StaticEffect node={img.i32(base)} "
                 f"is_cycled={img.boolean(base+4)} file_name={img.string(base+8)!r}")


def read_light(img, t, depth, i, base):
    t.add(depth, f"[{i}] StaticLight inode={img.i32(base)} color={fmt_floats(img.col4f(base+4))} "
                 f"atten_start={img.f32(base+20):.4g} atten_end={img.f32(base+24):.4g} "
                 f"texture={img.string(base+40)!r}")
    cb, cn = img.vector(base + 28, 12)            # chains: StaticLightAnimations, stride 12
    if cn:
        t.add(depth + 1, f"chains: {cn}")
        for c in range(cn):
            read_spline_interp(img, t, depth + 2, f"[{c}] color", cb + c * 12, 4)  # Rotation<4>


def read_leaf(img, t, depth, i, base):
    t.add(depth, f"[{i}] StaticLeaf inode={img.i32(base)} color={fmt_floats(img.col4f(base+4))} "
                 f"size={img.f32(base+20):.4g} texture={img.string(base+24)!r}")
    read_int_vector(img, t, depth + 1, "lods", base + 40)


def read_logo(img, t, depth, i, base):
    rmin = img.vec2(base)
    rmax = img.vec2(base + 8)
    t.add(depth, f"[{i}] sLogo TextureName={img.string(base+16)!r} "
                 f"rect=({fmt_floats(rmin)},{fmt_floats(rmax)}) angle={img.f32(base+28):.4g}")


def read_bound_sphere(img, t, depth, i, base):
    t.add(depth, f"[{i}] BoundSphere node_index={img.i32(base)} "
                 f"position={fmt_floats(img.vec3(base+4))} radius={img.f32(base+16):.4g}")


def read_box6f(img, t, depth, i, base):
    t.add(depth, f"[{i}] sBox6f min={fmt_floats(img.vec3(base))} max={fmt_floats(img.vec3(base+12))}")


def read_tvg(img, t, depth, i, base):
    # cTempVisibleGroup, stride 20
    t.add(depth, f"[{i}] cTempVisibleGroup visible_set={img.i32(base)} "
                 f"visibilities=0x{img.u32(base+4):08x} begin_polygon={img.i32(base+8)} "
                 f"num_polygon={img.i32(base+12)} visibilityNodeIndex={img.i32(base+16)}")


def read_bunch(img, t, depth, i, base):
    # StaticBunch, stride 44
    t.add(depth, f"[{i}] StaticBunch offset_polygon={img.i32(base)} num_polygon={img.i32(base+4)} "
                 f"offset_vertex={img.i32(base+8)} num_vertex={img.i32(base+12)} "
                 f"imaterial={img.i32(base+16)}")
    read_int_vector(img, t, depth + 1, "nodeIndices", base + 20)
    gb, gn = img.vector(base + 32, 20)            # visibleGroups: vector<cTempVisibleGroup>
    if gn:
        t.add(depth + 1, f"visibleGroups: {gn}")
        for g in range(gn):
            read_tvg(img, t, depth + 2, g, gb + g * 20)


def read_lod(img, t, depth, name, base):
    # StaticLod, stride 32:  ib@0 vb@4 blend_indices@8 bunches@12 sys_vb@24 sys_ib@28
    t.add(depth, f"{name}  blend_indices={img.i32(base+8)}")
    bb, bn = img.vector(base + 12, 44)            # bunches: StaticBunches
    t.add(depth + 1, f"bunches: {bn}")
    total_poly = 0
    for b in range(bn):
        read_bunch(img, t, depth + 2, b, bb + b * 44)
        total_poly += img.i32(bb + b * 44 + 4)
    if bn:
        t.add(depth + 1, f"(sum num_polygon over bunches = {total_poly})")


def read_generic_vector(img, t, depth, name, off, stride, reader):
    base, n = img.vector(off, stride)
    if n < 0:
        t.add(depth, f"{name}: <unserialized>")
        return
    t.add(depth, f"{name}: {n}")
    for i in range(n):
        reader(img, t, depth + 1, i, base + i * stride)


# --------------------------------- top level ---------------------------------
def parse(path):
    data = open(path, "rb").read()
    version, image_size = struct.unpack_from("<ii", data, 0)
    image = data[8:8 + image_size]
    off = 8 + image_size
    fixup_count = struct.unpack_from("<i", data, off)[0]; off += 4
    off += fixup_count * 4
    vtable_count = struct.unpack_from("<i", data, off)[0]; off += 4
    vtypes = []
    for _ in range(vtable_count):
        vo = struct.unpack_from("<i", data, off)[0]; off += 4
        end = data.index(b"\x00", off)
        vtypes.append((vo, data[off:end].decode("latin1")))
        off = end + 1

    img = Image(image)
    t = Tree()
    t.add(0, f"===== {path} =====")
    t.add(0, f"file header: version={version} imageSize={image_size} "
             f"fixups={fixup_count} vtableFixups={vtypes}")
    t.add(0, f"root: cStatic3dx (UnknownClass::m_cRef={img.i32(4)})")

    # ---- Static3dxBase subobject (starts at image offset 8) ----
    d = 1
    t.add(d, "--- Static3dxBase ---")
    t.add(d, f"version={img.i32(8)}  maxWeights={img.i32(12)}")
    t.add(d, f"fileName_={img.string(248)!r}")
    read_string_vector(img, t, d, "nonDeleteNodes", 16)
    read_string_vector(img, t, d, "logicNodes", 28)
    read_string_vector(img, t, d, "boundNodes", 40)

    read_generic_vector(img, t, d, "animationGroups_", 64, 48, read_animation_group)
    read_generic_vector(img, t, d, "animationChains_", 76, 28, read_animation_chain)
    read_generic_vector(img, t, d, "nodes", 52, 80, read_node)
    read_generic_vector(img, t, d, "visibilitySets_", 88, 36, read_vis_set)
    read_generic_vector(img, t, d, "materials", 100, 252, read_material)

    t.add(d, f"isBoundBoxInited={img.boolean(112)} boundRadius={img.f32(140):.4g}")
    t.add(d, f"boundBox min={fmt_floats(img.vec3(116))} max={fmt_floats(img.vec3(128))}")
    read_generic_vector(img, t, d, "localLogicBounds", 144, 24, read_box6f)
    read_generic_vector(img, t, d, "boundSpheres", 224, 20, read_bound_sphere)

    read_generic_vector(img, t, d, "logos", 156, 36, read_logo)   # StaticLogos.logos
    read_generic_vector(img, t, d, "effects", 168, 20, read_effect)
    read_generic_vector(img, t, d, "lights", 180, 56, read_light)
    read_generic_vector(img, t, d, "leaves", 192, 52, read_leaf)

    t.add(d, f"is_lod={img.boolean(204)} is_logic={img.boolean(205)} "
             f"is_old_model={img.boolean(206)} loaded={img.boolean(207)}")
    t.add(d, f"circle_shadow_enable={img.i32(208)} min={img.i32(212)} "
             f"height={img.i32(216)} radius={img.f32(220):.4g}")
    t.add(d, f"bump={img.boolean(236)} isUV2={img.boolean(237)} enableFur={img.boolean(238)}")
    t.add(d, f"cameraParams: camera_node_num={img.i32(240)} fov={img.f32(244):.4g}")

    tmb, tmn = img.vector(260, 4)   # tempMesh_ : vector<ShareHandle> (4-byte handles)
    l1b, l1n = img.vector(272, 4)
    l2b, l2n = img.vector(284, 4)
    t.add(d, f"tempMesh_ / tempMeshLod1_ / tempMeshLod2_ counts = {tmn} / {l1n} / {l2n}")

    # ---- cStatic3dx own members ----
    t.add(d, "--- cStatic3dx ---")
    read_generic_vector(img, t, d, "lods", 296, 32,
                        lambda im, tt, dp, i, b: read_lod(im, tt, dp, f"[{i}] StaticLod", b))
    read_lod(img, t, d, "debris (StaticLod)", 308)
    _, dbn = img.vector(340, 4)
    t.add(d, f"debrises (runtime cStaticSimply3dx*, expected empty): {dbn}")

    # VoxelBox @352
    vb_valid = img.boolean(352)
    _, vb_sz = img.memblock(404)
    t.add(d, f"voxelBox: valid={vb_valid} sizeLen={img.i32(356)} size={img.i32(360)} "
             f"mask={img.i32(364)} bufferBytes={vb_sz}")
    t.add(d + 1, f"scale={fmt_floats(img.vec3(368))} scaleInv={fmt_floats(img.vec3(380))} "
                 f"offset={fmt_floats(img.vec3(392))}")

    print(t)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: read_3dxG.py path/to/model.3dxG")
    parse(sys.argv[1])
