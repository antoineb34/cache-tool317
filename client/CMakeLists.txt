add_executable(client
    src/main.cpp
    src/ClientApp.cpp
)

target_link_libraries(client PRIVATE core SDL3::SDL3 OpenGL::GL)
