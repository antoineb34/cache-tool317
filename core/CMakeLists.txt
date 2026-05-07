add_library(core STATIC)

target_sources(core PRIVATE
    src/core.cpp
    src/Archive.cpp
    src/Buffer.cpp
    src/CacheReader.cpp
    src/DefinitionsLoader.cpp
    src/ItemDef.cpp
    src/LocDef.cpp
    src/NpcDef.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(core PUBLIC BZip2::BZip2)
