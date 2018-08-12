#ifndef LD42_TETROMINO_HPP
#define LD42_TETROMINO_HPP

#include "components.hpp"

#include <random>

template <typename Rng>
auto get_random_shape(Rng& rng) -> component::shape {
    const auto o_block = component::shape{{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}, {}, {0.5f, 0.5f}};
    const auto i_block = component::shape{{{{0, 0}, {0, 1}, {0, 2}, {0, 3}}}, {}, {0.5f, 1.5f}};
    const auto t_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {2, 0}}}, {}, {1.f, 0.f}};
    const auto j_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}, {}, {1.f, 1.f}};
    const auto l_block = component::shape{{{{0, 0}, {0, 1}, {0, 2}, {1, 0}}}, {}, {0.f, 1.f}};
    const auto s_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}, {}, {1.f, 0.f}};
    const auto z_block = component::shape{{{{0, 1}, {1, 1}, {1, 0}, {2, 0}}}, {}, {1.f, 0.f}};

    const component::shape blocks[7] = {
        o_block,
        i_block,
        t_block,
        j_block,
        l_block,
        s_block,
        z_block,
    };

    auto roll = std::uniform_int_distribution{0, 6};

    return blocks[roll(rng)];
}

#endif // LD42_TETROMINO_HPP
