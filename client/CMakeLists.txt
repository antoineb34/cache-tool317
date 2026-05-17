find_package(SDL3 REQUIRED CONFIG)

add_executable(client
    src/main.cpp
    src/Renderer3D.cpp
    src/Mat4.cpp
    src/Mesh.cpp
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(client PRIVATE
    core
    SDL3::SDL3
    OpenGL::GL
)
