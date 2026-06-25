add_library(Serialization STATIC
    BinaryArchive.cpp
    ComboStrings.cpp
    Dictionary.cpp
    EnumDescriptor.cpp
    GenericFileSelector.cpp
    InPlaceArchive.cpp
    LibraryWrapper.cpp
    MultiArchive.cpp
    Range.cpp
    RangedWrapper.cpp
    ResourceSelector.cpp
    StdAfx.cpp
    StringTableBase.cpp
    UTF8Wpapper.cpp
    XPrmArchive.cpp
)

target_include_directories(Serialization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Serialization PUBLIC EngineIncludes XUtil XMath)
