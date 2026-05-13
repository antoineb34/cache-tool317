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
    src/ModelDef.cpp
    src/TextureDef.cpp
    src/VersionList.cpp
    src/MapTerrain.cpp
    src/MapObjects.cpp
    src/MapRegion.cpp
    src/RegionRenderer2D.cpp
)
# Debug buffer bounds checking
option(DEBUG_BUFFER_CHECKS "Enable Buffer bounds checking" OFF)
if(DEBUG_BUFFER_CHECKS)
    target_compile_definitions(core PUBLIC DEBUG_BUFFER)
endif()
# Benchmark executables (optional)
if(BUILD_BENCHMARKS)
    add_executable(buffer_benchmark src/BufferBenchmark.cpp)
    target_link_libraries(buffer_benchmark PRIVATE core)
    add_executable(cache_benchmark src/CacheBenchmark.cpp)
    target_link_libraries(cache_benchmark PRIVATE core)
endif()
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(core PUBLIC BZip2::BZip2 ZLIB::ZLIB)
