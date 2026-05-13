find_package(SDL3 REQUIRED CONFIG)

add_executable(client
    src/main.cpp
)

target_compile_definitions(client PRIVATE GL_GLEXT_PROTOTYPES)

target_link_libraries(client
    core
    SDL3::SDL3
    OpenGL::GL
)
