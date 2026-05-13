find_package(SDL3 REQUIRED CONFIG)

add_executable(client
    src/main.cpp
    src/Math.h
    src/Camera.h
    src/Camera.cpp
    src/Shader.h
    src/Shader.cpp
    src/TextureManager.h
    src/TextureManager.cpp
    src/ModelRenderer.h
    src/ModelRenderer.cpp
    src/WorldRenderer.h
    src/WorldRenderer.cpp
)

target_compile_definitions(client PRIVATE GL_GLEXT_PROTOTYPES)

target_link_libraries(client
    core
    SDL3::SDL3
    OpenGL::GL
)
