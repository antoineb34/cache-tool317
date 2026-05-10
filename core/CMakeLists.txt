add_library(core STATIC)

target_sources(core PRIVATE
    src/core.cpp
    src/Archive.cpp
    src/Buffer.cpp
    src/CacheReader.cpp
    src/DefinitionsLoader.cpp
    src/FloDef.cpp
    src/IdkDef.cpp
    src/ItemDef.cpp
    src/LocDef.cpp
    src/MesAnimDef.cpp
    src/MesDef.cpp
    src/NpcDef.cpp
    src/ParamDef.cpp
    src/SeqDef.cpp
    src/SpotAnimDef.cpp
    src/VarbitDef.cpp
    src/VarpDef.cpp
    src/VersionList.cpp
    src/MapTerrain.cpp
    src/MapObjects.cpp
    src/MapRegion.cpp
    src/Model.cpp
    src/RegionRenderer2D.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(core PUBLIC BZip2::BZip2 ZLIB::ZLIB)
