add_executable(client
    src/main.cpp
    src/Mat4.cpp
    src/Shader.cpp
    src/ModelRenderer.cpp
    src/TextureLoader.cpp
)
target_include_directories(client PRIVATE include)
target_link_libraries(client PRIVATE core SDL3::SDL3 OpenGL::GL)
