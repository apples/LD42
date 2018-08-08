cmake_minimum_required(VERSION 3.2)
project(sushi)

add_library(sushi EXCLUDE_FROM_ALL
    src/sushi/sushi.hpp
    src/sushi/common.hpp
    src/sushi/gl.hpp
    src/sushi/gles_shim.hpp
    src/sushi/texture.hpp src/sushi/texture.cpp
    src/sushi/mesh.hpp src/sushi/mesh.cpp
    src/sushi/shader.hpp src/sushi/shader.cpp
    src/sushi/iqm.hpp src/sushi/iqm.cpp
    src/sushi/framebuffer.cpp src/sushi/framebuffer.hpp
    src/sushi/framebuffer_cubemap.cpp src/sushi/framebuffer_cubemap.hpp
    src/sushi/frustum.cpp src/sushi/frustum.hpp
)
set_property(TARGET sushi PROPERTY CXX_STANDARD 14)
target_include_directories(sushi PUBLIC src/)
target_link_libraries(sushi glm lodepng)
target_compile_definitions(sushi PUBLIC GLM_ENABLE_EXPERIMENTAL)

if (NOT EMSCRIPTEN)
    target_link_libraries(sushi glad)
endif()
