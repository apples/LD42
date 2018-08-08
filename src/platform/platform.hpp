#ifndef LD42_PLATFORM_HPP
#define LD42_PLATFORM_HPP

#include <string>

namespace platform {

void set_gl_version();

void load_gl_extensions();

std::string get_shader_path();

} // namespace platform

#endif // LD42_PLATFORM_HPP
