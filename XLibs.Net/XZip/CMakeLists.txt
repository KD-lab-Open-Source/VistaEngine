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
target_link_libraries(XZip PUBLIC EngineIncludes)
