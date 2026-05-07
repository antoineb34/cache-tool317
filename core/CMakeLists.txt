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
    src/SeqDef.cpp
    src/SpotAnimDef.cpp
    src/VarbitDef.cpp
    src/VarpDef.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(core PUBLIC BZip2::BZip2)
