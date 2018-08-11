#ifndef LD42_SPRITE_HPP
#define LD42_SPRITE_HPP

#include <sushi/mesh.hpp>
#include <sushi/texture.hpp>

inline auto make_sprite_mesh(const sushi::texture_2d& texture) -> sushi::static_mesh {
    auto left = -texture.width / 2.f;
    auto right = texture.width / 2.f;
    auto bottom = -texture.height / 2.f;
    auto top = texture.height / 2.f;

    return sushi::load_static_mesh_data(
        {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

inline auto make_solid_texture(float r, float g, float b) -> sushi::texture_2d {
    auto tex = sushi::create_uninitialized_texture_2d(1, 1, sushi::TexType::COLOR);
    const float pixel[4] = {r, g, b, 1.f};

    sushi::set_texture(0, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    return tex;
}

#endif // LD42_SPRITE_HPP
