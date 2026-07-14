# FFmpeg — the video decoder behind Video/.
#
# The game plays Bink 1 (.bik: the intro, the mission cutscenes, the briefing panels) and
# uses .avi as an animated-*texture* container (snowflakes, water rings, coast bubbles).
# Both were Windows-only: Bink through RAD's binkw32.dll, whose calls KD-lab stripped out of
# the source release, and AVI through Video-for-Windows. ffmpeg decodes both, on all three
# platforms, and is the only third-party decoder that does.
#
# What we ask of it is tiny, and the configure line says so: of ffmpeg's ~2000 components we
# build 2 demuxers and 3 decoders, and the result is under 3 MB of static library.
#
#   bink + avi      demuxers  the two containers
#   bink            decoder   Bink 1 video ("BIKi") — every .bik in the game is one
#   binkaudio_dct   decoder   the soundtrack inside a .bik (all of them are DCT, none RDFT)
#   rawvideo        decoder   the .avi textures: uncompressed 32-bit BGRA DIB frames
#   swscale                   YUV(A)420P -> BGRA, the byte order the textures want
#
# No protocols, deliberately: every file is opened through the game's own VFS with a custom
# AVIOContext (Video/VideoFile.cpp), so assets inside the .pak archives are reachable and
# ffmpeg never touches the filesystem itself. A protocol-less build is not an oversight —
# with the file protocol in, an avformat_open_input(path) would quietly bypass the VFS
# instead of failing, and only the loose copies of the assets would ever be found.
#
# --disable-x86asm keeps nasm off the dependency list on all three platforms. It costs decode
# speed we do not need: the largest video is 800x600@30 and the pure-C decoder handles that
# in a couple of ms a frame. Turn it back on (and add nasm to CI) only if that stops holding.
#
# FFmpeg has no CMake build, so this is its own configure + make wrapped in ExternalProject.
# On Windows that needs a shell: --toolchain=msvc makes cl.exe the compiler and lib.exe the
# archiver, so the libraries are MSVC-native and link into the Visual Studio build directly,
# but *running* configure still wants bash and make from MSYS2 (.github/workflows/windows.yaml).

include(ExternalProject)

set(FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg-install")

# Link order matters for static libraries, and it is the dependency order: avformat needs
# avcodec, both need avutil. The file names are the same on every platform — the win32 branch
# of ffmpeg's configure renames only the *shared* libraries, so even lib.exe emits libavcodec.a.
set(FFMPEG_COMPONENTS avformat avcodec swscale avutil)
foreach(component ${FFMPEG_COMPONENTS})
    list(APPEND FFMPEG_LIBRARIES "${FFMPEG_INSTALL_DIR}/lib/lib${component}.a")
endforeach()

set(FFMPEG_CONFIGURE_FLAGS
    --prefix=<INSTALL_DIR>
    --disable-everything          # ... and then add back, by name, only what we decode
    --enable-demuxer=bink,avi
    --enable-decoder=bink,binkaudio_dct,rawvideo
    --disable-autodetect          # no zlib/iconv/videotoolbox/... probing: a hermetic build
    --disable-programs            # no ffmpeg/ffplay/ffprobe binaries
    --disable-doc
    --disable-network
    --disable-avdevice
    --disable-avfilter
    --disable-swresample          # bink audio is planar float; interleaving it is ten lines
    --disable-postproc
    --enable-static
    --disable-shared
    --enable-pic                  # Linux links these into a PIE executable
    --disable-x86asm              # see above: nobody has to install nasm
    --disable-debug
)

if(WIN32)
    # MSYS2's bash and make, driving MSVC. Both are taken by ABSOLUTE PATH, and MSYS2's bin
    # directory is put on PATH for ffmpeg's build steps *only*. Neither half of that is fuss:
    #
    #  - A `make` found on PATH is not necessarily MSYS2's. The Windows CI image carries another
    #    GNU make, and a *native* one cannot resolve the POSIX source path ("/d/a/...") that
    #    configure — being a shell script — bakes into the Makefile it writes. It fails with
    #    "No rule to make target '/d/a/.../Makefile'", which is how this was found. NO_DEFAULT_PATH
    #    below is what stops a stray make from being picked up at all.
    #  - But MSYS2 does have to be on PATH while make runs, because ffmpeg's Makefile shells out
    #    to sed, awk and the rest. Scoping that to these commands keeps it away from the game's
    #    own link step, where MSYS2's coreutils link.exe would shadow MSVC's linker of the same
    #    name. ffmpeg itself is immune to that: its linker is the compat/windows/mslink wrapper,
    #    which looks for link.exe next to cl.exe rather than trusting PATH.
    #
    # And nothing here sets MSYS2_ARG_CONV_EXCL, which is the obvious thing to reach for and is
    # wrong. MSYS2 rewrites POSIX paths into Windows ones when an MSYS2 program spawns a native
    # one, and the MSVC build *depends* on it: make hands cl.exe a source path of "/d/a/...c",
    # which cl would otherwise read as an option ("ignoring unknown option", then "missing source
    # filename"). This is also why ffmpeg spells every MSVC flag with a dash -- -nologo, not
    # /nologo -- so that the same rewriting cannot mistake an option for a path. Turning the
    # conversion off breaks the build it is holding up.
    set(MSYS2_ROOT "$ENV{MSYS2_ROOT}" CACHE PATH
        "MSYS2 installation: ffmpeg's configure is a shell script and needs its bash and make")

    set(MSYS2_BIN_HINTS "${MSYS2_ROOT}/usr/bin" "$ENV{RUNNER_TEMP}/msys64/usr/bin" "C:/msys64/usr/bin")
    find_program(FFMPEG_BASH NAMES bash HINTS ${MSYS2_BIN_HINTS} NO_DEFAULT_PATH)
    find_program(FFMPEG_MAKE NAMES make HINTS ${MSYS2_BIN_HINTS} NO_DEFAULT_PATH)

    if(NOT FFMPEG_BASH OR NOT FFMPEG_MAKE)
        message(FATAL_ERROR
            "ffmpeg needs MSYS2's bash and make: its configure is a shell script (the compiler "
            "stays MSVC). Install MSYS2, run 'pacman -S make diffutils', and re-run cmake with "
            "-DMSYS2_ROOT=<msys2 dir> if it is somewhere other than C:/msys64.")
    endif()

    get_filename_component(MSYS2_BIN "${FFMPEG_MAKE}" DIRECTORY)
    # Forward slashes, NOT a native path: ExternalProject writes this command into a generated
    # .cmake script, and CMake re-reads "D:\a\_temp\msys64\usr\bin" as escape sequences -- the
    # script then fails to parse. Windows searches PATH entries with forward slashes quite
    # happily, and MSYS2's own tools convert them anyway.
    set(FFMPEG_ENV ${CMAKE_COMMAND} -E env
        --modify "PATH=path_list_prepend:${MSYS2_BIN}")
    set(FFMPEG_SHELL ${FFMPEG_BASH})
    list(APPEND FFMPEG_CONFIGURE_FLAGS --toolchain=msvc --target-os=win64 --arch=x86_64)
else()
    set(FFMPEG_ENV "")
    set(FFMPEG_SHELL "")
    set(FFMPEG_MAKE make)
    # Match the toolchain CMake was pointed at: CI builds Linux with clang, not the distro
    # gcc that ffmpeg's configure would otherwise find on its own.
    list(APPEND FFMPEG_CONFIGURE_FLAGS --cc=${CMAKE_C_COMPILER})

    # ... which on macOS means handing over the SDK as well. CMAKE_C_COMPILER there is the
    # compiler *inside* the Xcode toolchain, and that binary does not find the SDK by itself:
    # the /usr/bin/cc shim is what normally runs xcrun to locate one. Give ffmpeg's configure
    # the raw compiler without a sysroot and its very first check fails, with the compiler
    # unable to link so much as an empty main against libSystem.
    #
    # The host flags are not the same flags twice: ffmpeg compiles a few code generators to run
    # on the build machine, and for a native build its configure sets host_cc to our --cc while
    # leaving host_cflags alone -- so without these the *host* compiler check is the one that
    # fails, and it reports itself as "Host compiler lacks C11 support".
    if(APPLE AND CMAKE_OSX_SYSROOT)
        list(APPEND FFMPEG_CONFIGURE_FLAGS
            --extra-cflags=-isysroot${CMAKE_OSX_SYSROOT}
            --extra-ldflags=-isysroot${CMAKE_OSX_SYSROOT}
            --host-cflags=-isysroot${CMAKE_OSX_SYSROOT}
            --host-ldflags=-isysroot${CMAKE_OSX_SYSROOT})
    endif()
endif()

ExternalProject_Add(ffmpeg_external
    URL      https://ffmpeg.org/releases/ffmpeg-7.1.1.tar.xz
    URL_HASH SHA256=733984395e0dbbe5c046abda2dc49a5544e7e0e1e2366bba849222ae9e3a03b1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PREFIX      "${CMAKE_BINARY_DIR}/ffmpeg"
    INSTALL_DIR "${FFMPEG_INSTALL_DIR}"
    CONFIGURE_COMMAND ${FFMPEG_ENV} ${FFMPEG_SHELL} <SOURCE_DIR>/configure ${FFMPEG_CONFIGURE_FLAGS}
    BUILD_COMMAND     ${FFMPEG_ENV} ${FFMPEG_MAKE} -j4
    INSTALL_COMMAND   ${FFMPEG_ENV} ${FFMPEG_MAKE} install
    BUILD_BYPRODUCTS  ${FFMPEG_LIBRARIES}   # Ninja will not schedule a step whose output it cannot name
    LOG_CONFIGURE TRUE
    LOG_BUILD     TRUE
    LOG_INSTALL   TRUE
    LOG_OUTPUT_ON_FAILURE TRUE
)

# The libraries do not exist at configure time — ExternalProject builds them — so the imported
# targets point at where they *will* be, and the include directory has to be created now, or
# CMake rejects it as non-existent when it lands on an INTERFACE property.
file(MAKE_DIRECTORY "${FFMPEG_INSTALL_DIR}/include")

add_library(FFmpeg INTERFACE)
foreach(component ${FFMPEG_COMPONENTS})
    add_library(FFmpeg::${component} STATIC IMPORTED)
    set_target_properties(FFmpeg::${component} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_INSTALL_DIR}/lib/lib${component}.a")
    target_link_libraries(FFmpeg INTERFACE FFmpeg::${component})
endforeach()
target_include_directories(FFmpeg INTERFACE "${FFMPEG_INSTALL_DIR}/include")

# What ffmpeg's own .pc files declare underneath the libraries. None of it is a new system
# dependency: bcrypt (BCryptGenRandom, libavutil/random_seed.c) and the three Apple frameworks
# ship with the OS.
if(WIN32)
    target_link_libraries(FFmpeg INTERFACE bcrypt user32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(FFmpeg INTERFACE Threads::Threads m)
endif()
if(APPLE)
    target_link_libraries(FFmpeg INTERFACE
        "-framework CoreFoundation" "-framework CoreVideo" "-framework CoreMedia")
endif()
