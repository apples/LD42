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


#endif // LD42_SPRITE_HPP
