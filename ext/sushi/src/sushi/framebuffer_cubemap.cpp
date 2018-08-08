
#include "framebuffer_cubemap.hpp"

namespace sushi {

constexpr GLenum framebuffer_cubemap::FACES[];

framebuffer_cubemap::framebuffer_cubemap(int width, TexType type) :
    texture(create_uninitialized_texture_cubemap(width, type)),
    depth_texture(create_uninitialized_texture_2d(width, width, TexType::DEPTH)),
    framebuffers{
        make_unique_framebuffer(),
        make_unique_framebuffer(),
        make_unique_framebuffer(),
        make_unique_framebuffer(),
        make_unique_framebuffer(),
        make_unique_framebuffer(),
    }
{
    for (auto i=0; i<6; ++i) {
        sushi::set_framebuffer(framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FACES[i], texture.handle.get(), 0);
        glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture.handle.get(), 0);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, &buffers[0]);
    }

    set_framebuffer(nullptr);
}

const texture_cubemap& framebuffer_cubemap::get_cubemap() const {
    return texture;
}

const unique_framebuffer& framebuffer_cubemap::get_framebuffer(GLenum face) const {
    for (int i=0; i<6; ++i) {
        if (FACES[i] == face) {
            return framebuffers[i];
        }
    }
    throw std::runtime_error("Invalid GLenum!");
}

} // namespace sushi
