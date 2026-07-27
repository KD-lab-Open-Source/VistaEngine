# Building on Windows, Linux and macOS

The companion to `Render-PORTING.md`. That one registers what the *renderer* still owes;
this one records what it took to make the engine **build** on all three platforms, and
the traps that will bite again.

CI builds each platform from a clean checkout (`.github/workflows/`), so a change that
only compiles on one of them can no longer pass unnoticed.

## The CI jobs

Shaders decide what each job has to do:

| | shader compiler | notes |
|---|---|---|
| **Windows** | the SDK's own `dxc.exe` | HLSL → DXIL, nothing to install |
| **Linux** | SDL_shadercross + Microsoft's *prebuilt* DXC | only SPIRV-Cross and the CLI get compiled |
| **macOS** | SDL_shadercross + *vendored* DXC | no prebuilt DXC exists; a cold build compiles that LLVM fork (~12 min) |

macOS therefore caches the built `shadercross` CLI, keyed on the SDL_shadercross revision
pinned in `CMakeLists.txt`, and feeds it back via `-DSHADERCROSS_EXECUTABLE`. Cache **the
dylibs it links** (`libSDL3`, `libdxcompiler`, `libdxil`), not just the binary, and delete
the build tree before smoke-testing the cached copy — otherwise the CLI resolves its
libraries through the rpaths of a tree that will not exist on the next run.

Linux builds with **clang**, not the distro default: GCC accepts `-fms-extensions` but does
not bring `__super` with it, and the engine uses that keyword in ~180 files.

Don't pin the Visual Studio generator. `windows-latest` moved to VS 2026 mid-2026; naming a
version only moves the breakage to the next image bump. CMake picks the newest it finds.

## One source list per module

Every module's `CMakeLists.txt` used to have a `WIN32` branch feeding it a different set of
files. **They are all gone.** Windows compiles what everyone else compiles, and the Win32
originals stay in the tree as reference for whoever ports them properly:

- **DirectPlay** — removed from the Windows SDK; `<dplay8.h>` is missing *there* too.
- **Bink** (`PlayBink.cpp`) — draws through `d3d9.h`/`d3dx9.h`, and nothing constructs one.
- **the XERRHAND crash handler** — reads `CONTEXT.Eip/.Esp/.Ebp`, which a 64-bit CONTEXT
  does not have.
- **DirectSound**, **kdw** (the editors' 32-bit Win32 widget toolkit), **ConsoleWindow**,
  **SystemUtil**'s Win32 half, **VideoMemoryInformation** (asks D3D9), **joystick**
  (DirectInput), the DbgHelp stack walkers, **CDKey** (links a 32-bit VC7.1 `verifier.lib`,
  and its call has been commented out for years).

The only `WIN32` guards left are in the root `CMakeLists.txt`, and they are real platform
differences: the shader toolchain, `_HAS_STD_BYTE=0`, `/FIwindows.h`, and the fake headers.

Two include directories serve everyone now: `Platform/d3d9_compat` (D3D9 declarations — the
render headers still name its types, and on Windows this *deliberately shadows* the SDK's
`d3d9.h`, since a retired backend only has to parse) and `Network/dplay8.h`.

## AddressSanitizer

```
cmake -B build-asan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVISTA_ASAN=ON
```

**macOS only for now**, and the configure fails loudly anywhere else rather than ignoring the
flag. Windows would need MSVC's `/fsanitize=address` plus its `clang_rt` DLL staged beside
the executable, Linux its own `libasan` on the link line; neither has been tried.

The option is declared **below every `FetchContent_MakeAvailable`** in the root
`CMakeLists.txt` and that placement is the whole mechanism: `add_compile_options` reaches the
subdirectories added *after* it, so SDL, FreeType, miniaudio and DXC stay clean while every
engine module — `XLibs.Net` included, since `XZip` and `XBuffer` are where a buffer bug would
hide — is instrumented. ffmpeg is out of reach either way; it is an ExternalProject with its
own configure. Partial instrumentation is fine: the clean libraries report nothing of their
own, and Game's link pulls the runtime in.

Two things to know before reading a report:

- **`RelWithDebInfo` is `-O2`, which drops the frame pointer**, so the option adds
  `-fno-omit-frame-pointer` — without it half of every stack trace is unreadable.
- **Leak detection does not exist here.** LeakSanitizer is unsupported on Darwin
  (`detect_leaks=1` aborts with "not supported on this platform"); ASan on macOS finds
  overflows and use-after-free, not leaks.

The game reads its data from the working directory:

```
cd GameData && ASAN_OPTIONS=intercept_strstr=0 ../build-asan/Game/Game
```

**That option is not optional in a `Debug` build**, and it is the first thing this port trips
over. Without it the game appears to hang during `loadAllLibraries()` — the last line printed
is `SDLMinimapRenderer: …` and it sits there at 100% CPU. It is not hung and it is not a
deadlock: `XPrmIArchive::getToken` (`XPrmArchive.cpp:832`) runs `strstr(i, "\"")` once per
quoted literal while parsing the UI attributes, with `i` pointing into the whole remaining file
buffer. Native `strstr` stops at the first quote a few bytes on; ASan's interceptor measures
the *entire* haystack with `internal_strlen` first, to poison-check the range it might read. So
every literal pays a scan of everything after it and an O(n) parse turns into O(n²) —
`sample`(1) puts 2786 of 2795 samples in `internal_strlen`. `intercept_strstr=0` turns off that
one interceptor and leaves the rest of ASan intact; the only checking lost is on `strstr`'s own
reads. `RelWithDebInfo` has the same shape with a small enough constant to get through.

Reports go to **stderr**, which is where this engine's own logging goes too, so a report lands
in the log in the place it happened. ASan writes them with `write(2)` rather than stdio, so
nothing buffers them away; `ASAN_OPTIONS=log_path=/tmp/asan` diverts them to `/tmp/asan.<pid>`
if the interleaving gets in the way. Under `lldb` the process stops on the report either way.

## Traps, by platform

### Linux — case sensitivity

`Crc/crc.cpp` is really `CRC/crc.cpp`; `"StdAfx.h"` from `AI/` really opens
`Util/XMath/stdafx.h`, which is lowercase. macOS and Windows resolve any spelling; ext4
resolves exactly one.

**Use clang's `-Wnonportable-include-path`** — it diagnoses precisely this and its fix-its
name the real on-disk spelling. Do not hand-roll the check: I did, twice, and both were
wrong (matching dependency files against `git ls-files` case-*sensitively* drops exactly the
broken files; and an include is not "fine" because *some* `-I` root matches it — the compiler
searches roots in order and stops at the first hit).

### Linux — GNU ld

Apple's linker resolves archives in any order. GNU ld scans each once, in the order given,
and never goes back. Two consequences:

- **Declare every real dependency.** `XMath` serializes through `XStream`, so it depends on
  `XUtil` — that was always true and never declared.
- **Declare cycles too.** `Render` links `3dx`, and `3dx` calls back into `Render`
  (`Leaves`, `cAccessTexture`). Saying so makes CMake repeat both archives on the link line.

### Linux — glibc vs libc++

libc++ pulls headers in transitively; glibc does not. `<climits>` (`INT_MAX`, `USHRT_MAX`),
`<cstdarg>` (`va_start`), `<iterator>` (`back_inserter`), `<ios>` (`ios_base::failbit`) all
have to be included by the files that use them.

Also: `DT_UNKNOWN` is POSIX (`<dirent.h>`), and it collided with the engine's own enumerator.
The real fix was to stop force-including `<dirent.h>` into every translation unit —
`WindowsAPI.cpp` iterates directories with `<filesystem>` now.

`fpos_t` is an opaque struct on glibc. `ftell`/`fseek` when you want a byte offset.

`__argc`/`__argv` are MSVC globals that XUtil reads. macOS maps them onto Apple's
`_NSGetArgv()`; glibc has no equivalent, so `main()` captures them.

### Windows — the code was never 64-bit

**No inline assembly.** MSVC accepts none on x64. Every `__asm` block had a portable branch
already, guarded by `_CROSS_PLATFORM_` — which is only defined *off*-Windows, so the asm
branch was Windows's: `round()`, `fmodFast()`, `BitSR()`, `fastsqrtI()`, `FloatToInt()`,
`clampX()`, the RDTSC clock, `int 3`.

`round()` could not even be declared: as `int round(double)` it differs from the CRT's
`double round(double)` only by return type. It is the C library's now — which means it
returns a **double**, and `Vect2i(round(x), round(y))` is ambiguous between `Vect2i(int,int)`
and `Vect2i(float,float)`. Expect that one to keep surfacing.

`_DLL` is MSVC's flag for linking the DLL *runtime* (`/MD`), not for building a DLL.
`RENDER_API`/`KDW_API` keyed off it and so declared a static library's symbols
`__declspec(dllimport)`.

### Windows — MSVC is conforming now, clang only warns

Harvest these from a **clean** macOS build (an incremental one only re-emits warnings for
what it recompiled):

| clang warning | MSVC |
|---|---|
| `-Waddress-of-temporary` | **error** C2102 — 63 sites. `tempPtr()` (`Util/XTL/TempPtr.h`) binds the temporary to a reference parameter, which is what makes it addressable. |
| `-Wwritable-strings` | **error** C2440 — 104 sites, but they came from four *signatures* (`dprintf`, `RDWriteLog`, `StatisticalData`, `LoadTexture`). Fix the signature. |
| `-Wregister` | **error** C3878 — 55 sites. |

And without a clang counterpart: qualified member names inside their own class
(`cWaves::Init` declared in `cWaves`), default arguments repeated on an out-of-line template
member, `SerializationFactory` used with only a forward declaration in scope.

`#pragma comment(lib)` **with no library name** crashes MSVC outright (internal compiler
error C1001) — and clang had been emitting it into every object file as a malformed linker
option, the `-l missing <path>` warning at every macOS link.

### Windows — the Win32 API is assumed present

This is MSVC-origin code: its headers name `HWND`, `HANDLE` and `DWORD` freely, and each
module's `StdAfx.h` used to drag `windows.h` in for them. The stub translation units have no
`StdAfx.h`. So Windows force-includes `windows.h` into every C++ TU (`/FIwindows.h`) — the
mirror of the `WindowsAPI.h` force-include off-Windows.

That has consequences, and they are the price of the deal:

- `windows.h` brings **WinSock 1**, and XmlRpc uses WinSock 2. `WIN32_LEAN_AND_MEAN`, scoped
  to that target (globally it would also drop `ole2.h`, which `Runtime.cpp` needs).
- `dlgs.h` defines **`frm1`**, `edt1`, `stc1`… as control IDs. A local variable called `frm1`
  becomes an integer constant.
- `rpcndr.h` declares a global **`byte`**, which is ambiguous against `std::byte` wherever
  `using namespace std` is in scope. `_HAS_STD_BYTE=0`.

## Real bugs this turned up

Not portability defects — actual bugs, on every platform:

- **8 bytes written into a 4-byte read.** Containers streamed their `size()` into `XBuffer`,
  whose operators stop at `unsigned long` — 8 bytes on LP64, while the reader takes an `int`.
  The original 32-bit build was consistent. Fixed in the replay format (`UniverseX`),
  `ParameterSet` and `NParticleKey`; **the wire fields say `int32_t`/`uint32_t` now**, and the
  pattern is worth looking for wherever reader and writer sit in different files.
- **`%08lX` for a 32-bit field.** The first thing ASan reported, on the first run: `XGUID`
  printed its GUID with `"%08lX, %04hX, %04hX, {%02wX, …}"`, and under LP64 the `l` takes 64
  bits off the varargs for a 32-bit `Data1` — a 16-digit number, five bytes off the end of the
  80-byte buffer, and every argument after it shifted by one. `sscanf` read it back with
  `"%lx"` *into* `Data1`, writing eight bytes into four, over `Data2` and `Data3`. So every
  GUID this build wrote — the campaign progress in `passedMissions`, the mission headers — was
  garbage. It formats with `std::format` now, which takes each width from the argument's type;
  the text is the same canonical 78-character form the 32-bit build wrote.
- **The vendored zlib compiled against the system's `zlib.h`.** `XLibs.Net/XZip/zlib` was on
  nobody's include path, so minizip's `#include "zlib.h"` quietly resolved to
  `/usr/include/zlib.h` — a different zlib than the `.c` files beside it.
- **Optimised builds link differently.** A TU that references an inline or template entity it
  cannot see the definition of gets away with it at `-O0`, because some *other* TU emits an
  out-of-line copy. At `-O2` every caller inlines its own and the copy vanishes. CI builds
  `RelWithDebInfo`: **verify that configuration before believing a green build.**
