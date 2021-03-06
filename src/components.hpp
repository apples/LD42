#ifndef LD42_COMPONENTS_HPP
#define LD42_COMPONENTS_HPP

#include "sushi/sushi.hpp"

#include "json.hpp"
#include "scripting.hpp"
#include "entities.hpp"

#include <Meta.h>

#include <functional>
#include <string>
#include <type_traits>
#include <array>
#include <memory>
#include <chrono>
#include <optional>

#define REGISTER(NAME, ...)                                           \
        }                                                             \
        namespace meta {                                              \
        template <> constexpr auto registerName<component::NAME>() {  \
            return #NAME;                                             \
        }                                                             \
        template <> inline auto registerMembers<component::NAME>() {  \
            using comtype = component::NAME;                          \
            return members(__VA_ARGS__);                              \
        }                                                             \
        }                                                             \
        namespace component {
#define MEMBER(FIELD) member(#FIELD, &comtype::FIELD)

namespace component {

void register_components(sol::table& component_table);

struct net_id {
    ember_database::net_id id;
};

REGISTER(net_id,
         MEMBER(id))

struct position {
    float x = 0;
    float y = 0;
};

REGISTER(position,
         MEMBER(x),
         MEMBER(y))

struct velocity {
    float vx = 0;
    float vy = 0;
};

REGISTER(velocity,
         MEMBER(vx),
         MEMBER(vy))

struct aabb {
    float left = 0;
    float right = 0;
    float bottom = 0;
    float top = 0;
};

REGISTER(aabb,
         MEMBER(left),
         MEMBER(right),
         MEMBER(bottom),
         MEMBER(top))

struct script {
    std::string name;
};

REGISTER(script,
         MEMBER(name))

struct animation {
    std::string name;
    std::string cycle;
    int frame = 0;
    float t = 0;
    float offset_x = 0;
    float offset_y = 0;
    float rot = 0;
    float scale = 1;
};

REGISTER(animation,
         MEMBER(name),
         MEMBER(cycle),
         MEMBER(frame),
         MEMBER(t),
         MEMBER(offset_x),
         MEMBER(offset_y),
         MEMBER(rot),
         MEMBER(scale))

struct death_timer {
    double time = 0;
};

REGISTER(death_timer,
         MEMBER(time))

struct shape {
    std::array<glm::ivec2, 4> pieces;
    std::array<int, 4> colors;
    glm::vec2 pivot;
};

REGISTER(shape,
         MEMBER(pieces),
         MEMBER(colors),
         MEMBER(pivot))

struct board {
    std::array<std::array<std::optional<ember_database::net_id>, 10>, 22> grid;
    std::optional<ember_database::net_id> active;
    double next_tick = 0.0;
};

REGISTER(board,
         MEMBER(grid),
         MEMBER(active),
         MEMBER(next_tick))

struct block {
    int color;
};

REGISTER(block,
         MEMBER(color))

struct particle {
    glm::vec2 accel;
    float angle;
    float spin;
    int color;
};

REGISTER(particle,
         MEMBER(accel),
         MEMBER(angle),
         MEMBER(spin),
         MEMBER(color))

} //namespace component

#undef MEMBER
#undef REGISTER

#endif //LD42_COMPONENTS_HPP
