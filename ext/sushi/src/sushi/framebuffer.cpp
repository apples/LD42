//
// Created by Jeramy on 12/31/2015.
//

#include "framebuffer.hpp"

#include <numeric>

namespace {

void attach_colors(const std::vector<sushi::texture_2d>& texs) {
    for (int i=0; i<texs.size(); ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texs[i].handle.get(), 0);
    }

    std::vector<GLenum> buffers(texs.size());
    std::iota(begin(buffers), end(buffers), GL_COLOR_ATTACHMENT0);
    glDrawBuffers(buffers.size(), &buffers[0]);
}

void check_framebuffer_errors() {
    switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            throw std::runtime_error("Failed to create framebuffer: Texture size mismatch!");
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            throw std::runtime_error("Failed to create framebuffer: Incomplete attachments!");
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            throw std::runtime_error("Failed to create framebuffer: Missing attachments!");
        case GL_FRAMEBUFFER_UNSUPPORTED:
            throw std::runtime_error("Failed to create framebuffer: Unsupported format!");
        default:
            throw std::runtime_error("Failed to create framebuffer: Unknown error!");
    }
}

} // static

namespace sushi {

framebuffer create_framebuffer(std::vector<texture_2d> color_texs, texture_2d depth_tex) {
    auto width = std::numeric_limits<int>::max();
    auto height = std::numeric_limits<int>::max();

    for (auto& tex : color_texs) {
        width = std::min(width, tex.width);
        height = std::min(height, tex.height);
    }

    framebuffer rv = {
        width,
        height,
        make_unique_framebuffer(),
        std::move(color_texs),
        std::move(depth_tex)
    };

    set_framebuffer(rv);

    attach_colors(rv.color_texs);

    glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rv.depth_tex.handle.get(), 0);

    check_framebuffer_errors();

    set_framebuffer(nullptr);

    return rv;
}

framebuffer create_framebuffer(std::vector<texture_2d> color_texs) {
    auto width = std::numeric_limits<int>::max();
    auto height = std::numeric_limits<int>::max();

    for (auto& tex : color_texs) {
        width = std::min(width, tex.width);
        height = std::min(height, tex.height);
    }

    framebuffer rv = {
        width,
        height,
        make_unique_framebuffer(),
        std::move(color_texs),
        {}
    };

    set_framebuffer(rv);

    attach_colors(rv.color_texs);

    check_framebuffer_errors();

    set_framebuffer(nullptr);

    return rv;
}

} // namespace sushi
