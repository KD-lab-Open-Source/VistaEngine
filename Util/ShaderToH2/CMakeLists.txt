add_library(ShaderToH STATIC
    HLSL.cpp
    LuaExec.cpp
    ParamParse.cpp
    ProcessInclude.cpp
    ProcessString.cpp
    ShaderToH.cpp
    StdAfx.cpp
    saver/Saver.cpp
)

target_include_directories(ShaderToH PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ShaderToH PUBLIC EngineIncludes)
