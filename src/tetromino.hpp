#ifndef LD42_TETROMINO_HPP
#define LD42_TETROMINO_HPP

#include "components.hpp"

#include <random>
#include <algorithm>

template <typename Rng>
auto get_random_shape(Rng& rng, std::vector<component::shape>& bag) -> component::shape {
    const auto o_block = component::shape{{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}, {}, {0.5f, 0.5f}};
    const auto i_block = component::shape{{{{0, 0}, {0, 1}, {0, 2}, {0, 3}}}, {}, {0.5f, 1.5f}};
    const auto t_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {2, 0}}}, {}, {1.f, 0.f}};
    const auto j_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}, {}, {1.f, 1.f}};
    const auto l_block = component::shape{{{{0, 2}, {0, 1}, {0, 0}, {1, 0}}}, {}, {0.f, 1.f}};
    const auto s_block = component::shape{{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}, {}, {1.f, 0.f}};
    const auto z_block = component::shape{{{{0, 1}, {1, 1}, {1, 0}, {2, 0}}}, {}, {1.f, 0.f}};

    if (bag.empty()) {
        bag.push_back(o_block);
        bag.push_back(i_block);
        bag.push_back(t_block);
        bag.push_back(j_block);
        bag.push_back(l_block);
        bag.push_back(s_block);
        bag.push_back(z_block);

        std::shuffle(begin(bag), end(bag), rng);
    }

    auto roll_color = std::uniform_int_distribution{1, 2};

    auto block = bag.back();
    bag.pop_back();

    block.colors[0] = roll_color(rng);
    block.colors[3] = roll_color(rng);

    return block;
}

#endif // LD42_TETROMINO_HPP
