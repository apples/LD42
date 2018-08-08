//
// Created by Jeramy on 12/31/2015.
//

#ifndef SUSHI_FRAMEBUFFER_HPP
#define SUSHI_FRAMEBUFFER_HPP

#include "common.hpp"
#include "gl.hpp"

#include "texture.hpp"

#include <stdexcept>
#include <vector>

namespace sushi {

/// Deleter for OpenGL framebuffer objects.
struct framebuffer_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteFramebuffers(1, &buf);
    }
};

/// A unique handle to an OpenGL framebuffer object.
using unique_framebuffer = unique_gl_resource<framebuffer_deleter>;

/// Creates a unique OpenGL texture object.
/// \return A unique texture object.
inline unique_framebuffer make_unique_framebuffer() {
    GLuint buf;
    glGenFramebuffers(1, &buf);
    return unique_framebuffer(buf);
}

/// A framebuffer.
struct framebuffer {
    int width = 0;
    int height = 0;
    unique_framebuffer handle;
    std::vector<texture_2d> color_texs;
    texture_2d depth_tex;
};

/// Creates a framebuffer using the given textures.
/// \param color_texs Color buffer.
/// \param depth_tex Depth buffer.
/// \return The new framebuffer.
framebuffer create_framebuffer(std::vector<texture_2d> color_texs, texture_2d depth_tex);

/// Creates a framebuffer using the given textures.
/// \param color_texs Color buffers.
/// \return The new framebuffer.
framebuffer create_framebuffer(std::vector<texture_2d> color_texs);

/// Sets the given framebuffer as the current.
/// \param fb Framebuffer.
inline void set_framebuffer(const unique_framebuffer& fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb.get());
}

/// Sets the given framebuffer as the current.
/// \param fb Framebuffer.
inline void set_framebuffer(const framebuffer& fb) {
    set_framebuffer(fb.handle);
}

/// Sets the default framebuffer as the current.
inline void set_framebuffer(std::nullptr_t) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace sushi

#endif //SUSHI_FRAMEBUFFER_HPP
