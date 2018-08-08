//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_TEXTURE_HPP
#define SUSHI_TEXTURE_HPP

#include "gl.hpp"
#include "common.hpp"

#include <string>

/// Sushi
namespace sushi {

/// Deleter for OpenGL texture objects.
struct texture_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteTextures(1, &buf);
    }
};

/// A unique handle to an OpenGL texture object.
using unique_texture = unique_gl_resource<texture_deleter>;

/// Creates a unique OpenGL texture object.
/// \return A unique texture object.
inline unique_texture make_unique_texture() {
    GLuint buf;
    glGenTextures(1, &buf);
    return unique_texture(buf);
}

/// A 2D texture.
struct texture_2d {
    unique_texture handle;
    int width = 0;
    int height = 0;
};

/// A cubemap texture.
struct texture_cubemap {
    unique_texture handle;
};

enum class TexType : GLint {
    COLOR = GL_RGB,
    COLORA = GL_RGBA,
    DEPTH = GL_DEPTH_COMPONENT
};

/// Loads a 2D texture from a PNG file.
/// \param fname File name.
/// \param smooth Request texture smoothing.
/// \param wrap Request texture wrapping.
/// \param mipmaps Request mipmap generation.
/// \return The texture represented by the file, or an empty texture if a failure occurs.
texture_2d load_texture_2d(const std::string& fname, bool smooth, bool wrap, bool anisotropy, bool mipmaps, TexType type = TexType::COLORA);

/// Sets the active texture slot.
/// \param slot Slot index. Must be within the range `[0,GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`.
inline void set_active_texture(int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
}

/// Sets the texture for a slot.
/// \param slot Slot index. Must be within the range `[0,GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`.
/// \param tex The texture to bind.
inline void set_texture(int slot, const texture_2d& tex) {
    set_active_texture(slot);
    glBindTexture(GL_TEXTURE_2D, tex.handle.get());
}

/// Sets the texture for a slot.
/// \param slot Slot index. Must be within the range `[0,GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`.
/// \param tex The texture to bind.
inline void set_texture(int slot, const texture_cubemap& tex) {
    set_active_texture(slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex.handle.get());
}

texture_2d create_uninitialized_texture_2d(int width, int height, TexType type = TexType::COLOR);

texture_cubemap create_uninitialized_texture_cubemap(int width, TexType type = TexType::COLOR);

} // namespace sushi

#endif //SUSHI_TEXTURE_HPP
