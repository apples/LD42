#include "../platform.hpp"

#include "../../sdl.hpp"

#include <sushi/gl.hpp>

#include <stdexcept>

namespace platform {

void set_gl_version() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

void load_gl_extensions() {
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to load OpenGL extensions.");
    }
}

std::string get_shader_path() {
    return "data/shaders/GL4";
}

} // namespace platform
