#!/usr/bin/env python3
"""Convert Maelstrom's XPrm data to the schema this engine expects.

Maelstrom shipped on a VistaEngine revision from before ~Aug 2007; this tree is a
2008+ snapshot.  XPrmIArchive tolerates added, removed and renamed fields on its own
(`openNode` skips unknown names, `|new|old` aliases cover the renames), so the only
things that need rewriting are fields whose *C++ type* changed underneath the name.

Those were found by pairing every `ar.serialize(member, "wire", ...)` call site in
both trees with the member's declaration -- see RULES for the survivors.

The files are CP1251 with CRLF line endings and the engine reads them byte-for-byte,
so every substitution here is done on bytes: nothing re-encodes, re-wraps or
normalises a line it did not have to touch.
"""
import argparse, os, re, sys, collections

# --- What actually changed --------------------------------------------------------
#
# Each rule is (wire name, matcher, rewriter, why).  The matcher runs against the
# value text of `<name> = <value>;` and returns None when the value is already in
# our shape, so re-running the converter is a no-op.

def _float_to_int(v):
    """`3000.` / `2.5e+003` -> `3000`.  Only fires on a value carrying a fraction."""
    if re.fullmatch(rb'[+-]?\d+', v):
        return None                                  # already integral
    if not re.fullmatch(rb'[+-]?(\d+\.\d*|\.\d+|\d+\.)([eEdD][+-]?\d+)?', v):
        return None                                  # not a plain float literal
    return b'%d' % round(float(v.replace(b'd', b'e').replace(b'D', b'e')))

def _passability_to_bool(v):
    """RigidBodyPrm::PassabilityFlags collapsed to a bool.

    IMPASSABILITY = 0, PASSABILITY = 1 (Maelstrom Physics/RigidBodyPrm.h:81), and the
    two engines' defaults agree exactly -- Maelstrom sets waterPass = IMPASSABILITY /
    groundPass = PASSABILITY, ours sets false / true.
    """
    return {b'PASSABILITY': b'true', b'IMPASSABILITY': b'false'}.get(v)

RULES = [
    (b'steering_duration', _float_to_int,
     'RigidBodyPrm::steering_duration is float in Maelstrom, int here'),
    (b'groundPass', _passability_to_bool,
     'RigidBodyPrm::groundPass is PassabilityFlags in Maelstrom, bool here'),
    (b'waterPass', _passability_to_bool,
     'RigidBodyPrm::waterPass is PassabilityFlags in Maelstrom, bool here'),
]

# --- Indexed options are indices into a C++ list ----------------------------------
#
# Scripts/Content/GameOptions stores an option as `number = <index>` into a list that
# lives in C++ (Game/GameOptionsSerialization.cpp), not in the data.  The `comment`
# string beside it is only a label -- the engine never reads it.  Three of those lists
# gained or lost entries between the two revisions, so Maelstrom's saved indices select
# the wrong entry here:
#
#   OPTION_SCREEN_SIZE  25 = 1920*1080 there; index 25 here is 1680*945
#   OPTION_SHADOW        3 = High      there; we dropped Maelstrom's "Circle"
#   OPTION_LANGUAGE      0 = auto      there; we have no "Use Steam Language"
#
# The comment *is* Maelstrom's own list, which makes the conversion self-contained:
# resolve the index to a name against it, then look that name up in ours.  Both the
# number and the comment are rewritten, and that is what keeps the pass idempotent --
# leaving the old list in place would make a second run resolve the *new* index against
# the *old* names and convert a second time.

# Mirrors GameOptionsSerialization.cpp: resolutions_ (41), and the two translate lists.
OUR_OPTION_LISTS = {
    b'OPTION_SCREEN_SIZE':
    b'640*480|720*480|720*576|800*600|848*480|852*480|858*484|960*600|1024*600|1024*768|'
    b'1024*800|1088*612|1152*864|1280*720|1280*768|1280*800|1280*960|1280*1024|1360*768|'
    b'1366*768|1400*1050|1440*900|1600*900|1600*1024|1600*1200|1680*945|1680*1050|1920*1080|'
    b'1920*1200|1920*1440|2048*1080|2048*1536|2560*1600|2560*2048|3200*2048|3200*2400|'
    b'3840*2400|5120*4096|6400*4096|6400*4800|7680*4800',
    b'OPTION_SHADOW':   b'Disabled|Low|High',
    b'OPTION_LANGUAGE': b'English|Russian|German|French|Spanish|Italian',
}

# Names with no counterpart here, and the index to settle on instead.
OPTION_FALLBACK = {
    (b'OPTION_LANGUAGE', b'Use Steam Language'): 0,   # no auto-detect; English
    (b'OPTION_SHADOW',   b'Circle'):             1,   # circle shadows gone; Low
}

OPTION_BLOCK = re.compile(
    rb'(type[ \t]*=[ \t]*(OPTION_[A-Z_0-9]+);[ \t]*\r?\n'
    rb'[ \t]*number[ \t]*=[ \t]*)(-?\d+)(;[ \t]*\r?\n'
    rb'[ \t]*comment[ \t]*=[ \t]*")([^"]*)(";)')

def convert_options(data, counts):
    """Renumber indexed options against our lists, and carry our list into the comment."""
    def sub(m):
        option, ours = m.group(2), OUR_OPTION_LISTS.get(m.group(2))
        if ours is None:
            return m.group(0)
        theirs = m.group(5)
        if theirs == ours:
            return m.group(0)                        # already in our shape
        old = int(m.group(3))
        names = theirs.split(b'|')
        if not 0 <= old < len(names):
            return m.group(0)                        # not an index we can resolve
        name = names[old]
        our_names = ours.split(b'|')
        new = our_names.index(name) if name in our_names \
            else OPTION_FALLBACK.get((option, name))
        if new is None:
            return m.group(0)
        counts[option.decode()] += 1
        return m.group(1) + b'%d' % new + m.group(4) + ours + m.group(6)

    return OPTION_BLOCK.sub(sub, data)

# --- The font library ------------------------------------------------------------
#
# Maelstrom rasterised its fonts offline: Scripts/Content/UI_FontLibrary names logical
# fonts ("MAEL_small") that resolve to glyph atlases in cacheData/Fonts/*.xfont+.tga,
# and the distribution contains no TTF at all.  By 2008 the engine had moved to
# FreeType, reading a real .ttf named by Scripts/Content/UI_FontAttributes
# (UserInterface/UI_Types.cpp:68) -- a different file name *and* a different shape:
# ours is a plain StringTable<UI_LibFont> with a nested `font` block, Maelstrom's is a
# StringTableBasePolymorphic holding `second = "class UI_Font"`.
#
# Nothing in the data can bridge that, so this substitutes a TTF for the atlases.  The
# glyph shapes will not be Maelstrom's; matching those means teaching the engine to
# read .xfont, which is a code change, not a conversion.
FONT_ENTRY = '''		{
			name = "%(name)s";
			font = {
				fontFile = "%(ttf)s";
				fontSize = %(size)d;
				hinting = DEFAULT;
				aaMin = 8;
				aaMax = 10;
				inBox = false;
			};
		}'''

def build_font_attributes(src_bytes, ttf_path):
    """Translate Maelstrom's UI_FontLibrary into our UI_FontAttributes."""
    entries = re.findall(rb'first\s*=\s*"([^"]*)"\s*;\s*'
                         rb'second\s*=\s*"class UI_Font"\s*\{(.*?)\}', src_bytes, re.S)
    fonts = []
    for name, body in entries:
        m = re.search(rb'fontSize_\s*=\s*(\d+)', body)
        fonts.append((name.decode('cp1251'), int(m.group(1)) if m else 16))
    if not fonts:
        return None
    header = ',\n'.join('\t\t"%s"' % n for n, _ in fonts)
    strings = ',\n'.join(FONT_ENTRY % {'name': n, 'size': s,
                                       'ttf': ttf_path.replace('\\', '\\\\')}
                         for n, s in fonts)
    text = ('Version = 0;\nUI_FontLibrary = {\n\theader = {\n\t\t%d;\n%s\n\t};\n'
            '\tstrings = {\n\t\t%d;\n%s\n\t};\n};\n'
            % (len(fonts), header, len(fonts), strings))
    return text.encode('cp1251'), fonts

# Text XPrm data lives in these places; everything else in the tree is binary.
TEXT_DIRS = ('Scripts',)
TEXT_SUFFIXES = ('.spg', '.cls', '.tdb', '.set', '.scr')

def is_text_data(root, path):
    rel = os.path.relpath(path, root)
    head = rel.split(os.sep)[0]
    if head in TEXT_DIRS:
        return os.path.splitext(path)[1].lower() not in ('.tga', '.dds', '.bmp', '.ttf',
                                                         '.3dx', '.dat', '.cur', '.exe')
    return path.lower().endswith(TEXT_SUFFIXES)

def convert(data, counts):
    """Apply every rule to one file's bytes.  Returns the new bytes."""
    for name, rewrite, _why in RULES:
        pattern = re.compile(rb'(^[ \t]*' + re.escape(name) + rb'[ \t]*=[ \t]*)'
                             rb'([^;\r\n]*?)([ \t]*;)', re.M)

        def sub(m):
            new = rewrite(m.group(2).strip())
            if new is None:
                return m.group(0)
            counts[name.decode()] += 1
            return m.group(1) + new + m.group(3)

        data = pattern.sub(sub, data)

    return convert_options(data, counts)

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('root', help='Maelstrom data root (the dir holding Scripts/ and Resource/)')
    ap.add_argument('--apply', action='store_true',
                    help='rewrite the files in place (default: report only)')
    ap.add_argument('--font', metavar='TTF',
                    help='engine-relative .ttf to substitute for Maelstrom\'s glyph '
                         'atlases, e.g. "Resource\\\\UI\\\\Fonts\\\\ARIALNB2.ttf"')
    args = ap.parse_args()

    counts = collections.Counter()
    files = touched = 0
    for dp, dns, fns in os.walk(args.root):
        dns[:] = [d for d in dns if not d.startswith('.')]
        for fn in fns:
            path = os.path.join(dp, fn)
            if not is_text_data(args.root, path):
                continue
            files += 1
            with open(path, 'rb') as fh:
                data = fh.read()
            before = dict(counts)
            new = convert(data, counts)
            if new != data:
                touched += 1
                changed = sum(counts.values()) - sum(before.values())
                print('  %-70s %d' % (os.path.relpath(path, args.root), changed))
                if args.apply:
                    # The tree may be a symlink farm over a read-only pristine copy;
                    # writing through the link would edit the original, so drop it.
                    if os.path.islink(path):
                        os.unlink(path)
                    with open(path, 'wb') as fh:
                        fh.write(new)

    # The font library is a translation, not a substitution, so it is handled apart.
    src = os.path.join(args.root, 'Scripts', 'Content', 'UI_FontLibrary')
    dst = os.path.join(args.root, 'Scripts', 'Content', 'UI_FontAttributes')
    if os.path.exists(src) and not os.path.exists(dst):
        if not args.font:
            print('\n  Scripts/Content/UI_FontLibrary needs translating to '
                  'UI_FontAttributes, but Maelstrom ships no .ttf --\n'
                  '  re-run with --font <engine-relative .ttf> to pick a substitute.')
        else:
            with open(src, 'rb') as fh:
                built = build_font_attributes(fh.read(), args.font)
            if built:
                blob, fonts = built
                print('\n  Scripts/Content/UI_FontAttributes  <- UI_FontLibrary  (%s)'
                      % ', '.join('%s @%d' % f for f in fonts))
                if args.apply:
                    with open(dst, 'wb') as fh:
                        fh.write(blob)

    # The global trigger chain moved into a Triggers/ subdirectory between the two
    # revisions.  It is what starts the game: its "Start Main Menu" trigger carries the
    # ActionStartMission that loads Resource\Worlds\Menu.spg, and every screen the game
    # ever shows follows from that.  GameShell::init loads exactly one path, so with the
    # file a level up the chain is simply empty -- nothing starts, no screen is ever
    # selected, and the game sits on a black window.  Copy rather than move: the engine
    # is the only reader of the new location, and leaving the original in place keeps the
    # tree readable next to Maelstrom's own source.
    src = os.path.join(args.root, 'Scripts', 'Content', 'GlobalTrigger.scr')
    dst = os.path.join(args.root, 'Scripts', 'Content', 'Triggers', 'GlobalTrigger.scr')
    if os.path.exists(src) and not os.path.exists(dst):
        print('\n  Scripts/Content/Triggers/GlobalTrigger.scr  <- Scripts/Content/GlobalTrigger.scr')
        if args.apply:
            with open(src, 'rb') as fh:
                blob = fh.read()
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, 'wb') as fh:
                fh.write(blob)

    print('\nscanned %d text files, %d needed changes%s'
          % (files, touched, '' if args.apply else '  (dry run -- pass --apply to write)'))
    for name, rewrite, why in RULES:
        print('  %-22s %5d  -- %s' % (name.decode(), counts[name.decode()], why))
    for option in sorted(OUR_OPTION_LISTS):
        n = counts[option.decode()]
        if n:
            print('  %-22s %5d  -- index into a C++ list that changed shape'
                  % (option.decode(), n))
    return 0

if __name__ == '__main__':
    sys.exit(main())
