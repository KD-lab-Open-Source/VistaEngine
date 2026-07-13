add_library(XZip STATIC
    FileUtils.cpp
    XZip.cpp
    zlib/adler32.c
    zlib/compress.c
    zlib/contrib/minizip/ioapi.c
    zlib/contrib/minizip/unzip.c
    zlib/contrib/minizip/zip.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/infback.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/zutil.c
)

target_include_directories(XZip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# zlib.h ships right here, next to the zlib sources we compile above, but nothing put
# its directory on the include path: minizip's unzip.h asks for "zlib.h" and, on macOS
# and Linux, quietly got the *system* one from /usr/include — a different zlib than the
# .c files being compiled beside it. Windows has no system zlib, so there it just failed.
target_include_directories(XZip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
target_link_libraries(XZip PUBLIC EngineIncludes)
