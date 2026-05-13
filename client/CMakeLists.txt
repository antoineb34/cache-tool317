find_package(SDL3 REQUIRED CONFIG)

add_executable(client
    src/main.cpp
    src/App.cpp
    src/WireframeMeshRenderer.cpp
)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(client PRIVATE GL_GLEXT_PROTOTYPES)

target_link_libraries(client
    SDL3::SDL3
    OpenGL::GL
)