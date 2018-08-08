#ifndef SUSHI_FRAMEBUFFER_CUBEMAP_HPP
#define SUSHI_FRAMEBUFFER_CUBEMAP_HPP

#include "framebuffer.hpp"
#include "texture.hpp"

#include <array>
#include <stdexcept>

namespace sushi {

/// A texture_cubemap with a framebuffer for each face.
class framebuffer_cubemap {
public:
    static constexpr GLenum FACES[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    /// Constructs a cubemap where each face is width*width.
    /// Each face of the cubemap is bound to a framebuffer as COLOR0.
    /// A single depth buffer is bound across all the framebuffers.
    /// \param width Width and height of each face.
    /// \param type Texture type for cubemap.
    framebuffer_cubemap(int width, TexType type);

    /// Gets the cubemap.
    /// \return The cubemap.
    const texture_cubemap& get_cubemap() const;

    /// Gets the framebuffer for a face.
    /// \param face One of FACES.
    /// \return The framebuffer for the face.
    const unique_framebuffer& get_framebuffer(GLenum face) const;

private:
    texture_cubemap texture;
    texture_2d depth_texture;
    std::array<unique_framebuffer,6> framebuffers;
};

} // namespace sushi

#endif // SUSHI_FRAMEBUFFER_CUBEMAP_HPP
