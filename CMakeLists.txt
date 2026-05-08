cmake_minimum_required(VERSION 3.20)
project(cache-tool317 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binaries to a predictable place
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

add_subdirectory(core)
add_subdirectory(tool)
add_subdirectory(client)
