# Higher-level render helpers (post effects, field-of-view/fog map, Flash UI
# player, stream interpolation). Render-visual like Water: the sources include
# the D3D9 render headers, which resolve against the expanded fake d3d9.h
# (parse-only) + the Render stub LIB at link.
set(VISTARENDER_SOURCES
    stdafx.cpp
    Field.cpp
    FieldOfView.cpp
    Flash.cpp
    StreamInterpolation.cpp
    postEffects.cpp
)

add_library(VistaRender STATIC ${VISTARENDER_SOURCES})
target_link_libraries(VistaRender PUBLIC EngineIncludes Util Terra)
