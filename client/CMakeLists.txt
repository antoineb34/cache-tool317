add_executable(client
    src/main.cpp
)
target_include_directories(client PRIVATE include)
target_link_libraries(client PRIVATE core SDL3::SDL3 OpenGL::GL)
