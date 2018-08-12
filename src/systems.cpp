#include "systems.hpp"

#include "components.hpp"
#include "tetromino.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <sol.hpp>
#include <sushi/frustum.hpp>
#include <sushi/mesh.hpp>
#include <sushi/shader.hpp>
#include <sushi/texture.hpp>

#include <iostream>

namespace systems {

void movement(ld42_engine& engine, double delta) {
    engine.entities.visit([&](component::position& pos, const component::velocity& vel) {
        pos.x += vel.vx * delta;
        pos.y += vel.vy * delta;
    });
}

void collision(ld42_engine& engine, double delta) {
    using DB = ember_database;

    struct entity_info {
        DB::ent_id eid;
        component::aabb aabb;
    };

    struct collision_manifold {
        DB::ent_id eid1;
        DB::ent_id eid2;
        component::aabb region;
    };

    auto intersects = [](const component::aabb& a, const component::aabb& b) {
        return a.left < b.right &&
               b.left < a.right &&
               a.bottom < b.top &&
               b.top < a.bottom;
    };

    std::vector<entity_info> world;
    std::vector<const entity_info*> axis_list;
    std::vector<collision_manifold> collisions;

    world.reserve(engine.entities.size());
    axis_list.reserve(engine.entities.size());

    // Create world list
    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, const component::aabb& aabb) {
        auto info = entity_info{};
        info.eid = eid;
        info.aabb.left = pos.x + aabb.left;
        info.aabb.right = pos.x + aabb.right;
        info.aabb.bottom = pos.y + aabb.bottom;
        info.aabb.top = pos.y + aabb.top;
        world.push_back(info);
    });

    // Sort along X-axis
    std::sort(begin(world), end(world), [](const entity_info& a, const entity_info& b) {
        return a.aabb.left < a.aabb.right;
    });

    // Sweep
    for (const auto& info : world) {
        // Prune
        axis_list.erase(
            std::remove_if(begin(axis_list), end(axis_list), [&](const auto& other_info) {
                return other_info->aabb.right < info.aabb.left;
            }),
            end(axis_list));

        // Check pairs
        for (const auto& other_info : axis_list) {
            auto manifold = collision_manifold{};
            manifold.eid1 = info.eid;
            manifold.eid2 = other_info->eid;
            manifold.region.left = std::max(info.aabb.left, other_info->aabb.left);
            manifold.region.right = std::min(info.aabb.right, other_info->aabb.right);
            manifold.region.bottom = std::max(info.aabb.bottom, other_info->aabb.bottom);
            manifold.region.top = std::min(info.aabb.top, other_info->aabb.top);

            if (manifold.region.left < manifold.region.right && manifold.region.bottom < manifold.region.top) {
                collisions.push_back(manifold);
            }
        }
    }

    // Call handlers
    for (auto& collision : collisions) {
        auto call_script = [&](DB::ent_id eid1, DB::ent_id eid2, const component::aabb& aabb) {
            if (engine.entities.has_component<component::script>(eid1)) {
                auto& script = engine.entities.get_component<component::script>(eid1);
                auto script_ptr = engine.resources.environment_cache.get(script.name);
                auto on_collide = (*script_ptr)["on_collide"];
                if (on_collide.valid()) {
                    on_collide(eid1, eid2, aabb);
                }
            }
        };
        call_script(collision.eid1, collision.eid2, collision.region);
        call_script(collision.eid2, collision.eid1, collision.region);
    }
}

void scripting(ld42_engine& engine, double delta) {
    using DB = ember_database;
    engine.entities.visit([&](DB::ent_id eid, const component::script& script) {
        auto update = (*engine.resources.environment_cache.get(script.name))["update"];
        if (update.valid()) {
            update(eid, delta);
        }
    });
}

void death_timer(ld42_engine& engine, double delta) {
    using DB = ember_database;
    engine.entities.visit([&](DB::ent_id eid) {
        if (engine.entities.has_component<component::death_timer>(eid)) {
            auto& timer = engine.entities.get_component<component::death_timer>(eid);
            timer.time -= delta;
            if (timer.time <= 0) {
                if (engine.entities.has_component<component::script>(eid)) {
                    auto& script = engine.entities.get_component<component::script>(eid);
                    auto env_ptr = engine.resources.environment_cache.get(script.name);
                    auto on_death = (*env_ptr)["on_death"];
                    if (on_death.valid()) {
                        on_death(eid);
                    }
                }
                engine.entities.destroy_entity(eid);
            }
        }
    });
}

void render(ld42_engine& engine, double delta) {
    using DB = ember_database;
    auto proj = glm::ortho(-8.f, 56.f/3.f, 0.f, 20.f, 10.f, -10.f);
    auto view = glm::mat4(1.f);
    auto frustum = sushi::frustum(proj * view);

    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, const component::shape& shape) {
        using namespace std::literals;

        for (int i = 0; i < 4; ++i) {
            auto piece_modelmat = glm::translate(glm::mat4(1), glm::vec3(glm::vec2(pos.x, pos.y) + glm::vec2(shape.pieces[i]), 0.f));
            auto tint = glm::vec4{1, 1, 1, 1};

            sushi::set_texture(0, *engine.resources.texture_cache.get("block_"s + std::to_string(shape.colors[i])));
            sushi::set_uniform("normal_mat", glm::inverseTranspose(view * piece_modelmat));
            sushi::set_uniform("MVP", (proj * view * piece_modelmat));
            sushi::set_uniform("tint", tint);
            sushi::draw_mesh(engine.sprite_mesh);
        }
    });
}

void board_tick(ld42_engine& engine, double delta) {
    engine.entities.visit([&](component::board& board) {
        if (board.active) {
            auto active = engine.entities.get_entity(*board.active);
            auto& pos = engine.entities.get_component<component::position>(active);
            auto& shape = engine.entities.get_component<component::shape>(active);
            auto nid = engine.entities.get_component<component::net_id>(active).id;

            // Movement

            auto can_move = [&](int dir) {
                for (int i = 0; i < 4; ++i) {
                    auto x = pos.x + shape.pieces[i].x + dir;
                    if (x < 0 || x >= board.grid[0].size() || board.grid[pos.y + shape.pieces[i].y][x]) {
                        return false;
                    }
                }
                return true;
            };

            if (engine.input_table.get_or("left_pressed", false) && can_move(-1)) {
                pos.x -= 1;
            }

            if (engine.input_table.get_or("right_pressed", false) && can_move(1)) {
                pos.x += 1;
            }

            // Rotation
            if (engine.input_table.get_or("up_pressed", false)) {
                auto new_shape = shape;
                auto rotmat = glm::mat2({0.f, -1.f}, {1.f, 0.f});
                for (int i=0; i<4; ++i) {
                    new_shape.pieces[i] = glm::ivec2(glm::round(rotmat * (glm::vec2(new_shape.pieces[i]) - new_shape.pivot) + new_shape.pivot));
                }
                auto is_good = [&] {
                    for (int i = 0; i < 4; ++i) {
                        auto x = pos.x + new_shape.pieces[i].x;
                        auto y = pos.y + new_shape.pieces[i].y;
                        if (x < 0 || x >= board.grid[0].size() || y < 0 || y >= board.grid.size() || board.grid[y][x]) {
                            return false;
                        }
                    }
                    return true;
                };
                if (is_good()) {
                    shape = new_shape;
                }
            }

            board.next_tick -= delta;

            if (engine.input_table.get_or("down", false)) {
                board.next_tick -= delta;
            }

            if (board.next_tick <= 0.0) {
                board.next_tick += 0.25;
                auto should_lock = [&] {
                    for (int i = 0; i < 4; ++i) {
                        if (pos.y + shape.pieces[i].y - 1 < 0 || board.grid[pos.y + shape.pieces[i].y - 1][pos.x + shape.pieces[i].x]) {
                            return true;
                        }
                    }
                    return false;
                };

                if (should_lock()) {
                    for (int i = 0; i < 4; ++i) {
                        board.grid[pos.y + shape.pieces[i].y][pos.x + shape.pieces[i].x] = nid;
                    }
                    auto active = engine.entities.create_entity();
                    engine.entities.create_component(active, component::position{5, 19});
                    engine.entities.create_component(active, get_random_shape(engine.rng));
                    board.active = engine.entities.get_component<component::net_id>(active).id;
                } else {
                    pos.y -= 1;
                }
            }
        }
    });
}

}  //namespace systems
