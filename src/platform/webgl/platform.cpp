#include "../platform.hpp"

#include "../../sdl.hpp"

namespace platform {

void set_gl_version() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
}

void load_gl_extensions() {
    return;
}

std::string get_shader_path() {
    return "data/shaders/GLES2";
}

} // namespace platform
