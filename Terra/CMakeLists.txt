add_library(Terra STATIC
    QSWorldsMgr.cpp
    TerrainType.cpp
    Toolser.cpp
    bitGen.cpp
    break.cpp
    fastmath.cpp
    geo.cpp
    geo2.cpp
    geoNet.cpp
    grid.cpp
    pn.cpp
    pnint.cpp
    pnt.cpp
    procedurMap.cpp
    quantizer.cpp
    road.cpp
    scalingEngine.cpp
    stdafxTr.cpp
    terPoligon.cpp
    terToolDispatcher.cpp
    terTools.cpp
    tgai.cpp
    tools.cpp
    undoDispatcher.cpp
    vbitmap.cpp
    vmap4vi.cpp
    worldFileDispatcher.cpp
    worldGen.cpp
)

target_link_libraries(Terra PUBLIC EngineIncludes Util)
