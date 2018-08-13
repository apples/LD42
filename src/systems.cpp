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
    using namespace std::literals;
    using DB = ember_database;

    auto proj = glm::ortho(-8.f, 56.f/3.f, -0.5f, 19.5f, 10.f, -10.f);
    auto view = glm::mat4(1.f);
    auto frustum = sushi::frustum(proj * view);

    auto draw_block = [&](glm::vec2 pos, int color) {
        auto piece_modelmat = glm::translate(glm::mat4(1), glm::vec3(pos, 0.f));
        auto tint = glm::vec4{1, 1, 1, 1};

        sushi::set_texture(0, *engine.resources.texture_cache.get("block_"s + std::to_string(color)));
        sushi::set_uniform("normal_mat", glm::inverseTranspose(view * piece_modelmat));
        sushi::set_uniform("MVP", (proj * view * piece_modelmat));
        sushi::set_uniform("tint", tint);
        sushi::draw_mesh(engine.sprite_mesh);
    };

    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, const component::shape& shape) {
        for (int i = 0; i < 4; ++i) {
            auto xy = glm::vec2(pos.x, pos.y) + glm::vec2(shape.pieces[i]);
            draw_block(xy, shape.colors[i]);
        }
    });

    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, const component::block& block) {
        draw_block(glm::vec2(pos.x, pos.y), block.color);
    });

    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, component::velocity& vel, component::particle& particle) {
        auto modelmat = glm::mat4(1);
        modelmat = glm::translate(modelmat, glm::vec3(pos.x, pos.y, 0.f));
        modelmat = glm::rotate(modelmat, particle.angle, glm::vec3(0.f, 0.f, 1.f));
        modelmat = glm::scale(modelmat, glm::vec3(0.5f, 0.5f, 0.5f));
        auto tint = glm::vec4{1, 1, 1, 1};

        sushi::set_texture(0, *engine.resources.texture_cache.get("block_"s + std::to_string(particle.color)));
        sushi::set_uniform("normal_mat", glm::inverseTranspose(view * modelmat));
        sushi::set_uniform("MVP", (proj * view * modelmat));
        sushi::set_uniform("tint", tint);
        sushi::draw_mesh(engine.sprite_mesh);

        vel.vx += particle.accel.x * delta;
        vel.vy += particle.accel.y * delta;
        particle.angle += particle.spin * delta;

        if (pos.y < -1.f) {
            engine.entities.destroy_entity(eid);
        }
    });
}

void board_tick(ld42_engine& engine, double delta) {
    auto break_block = [&](ember_database::net_id nid) {
        auto eid = engine.entities.get_entity(nid);
        const auto& pos = engine.entities.get_component<component::position>(eid);
        const auto& block = engine.entities.get_component<component::block>(eid);

        auto spawn_particle = [&](const component::velocity& vel) {
            auto particle = engine.entities.create_entity();
            engine.entities.create_component(particle, component::position{pos});
            engine.entities.create_component(particle, component::velocity{vel});
            engine.entities.create_component(particle, component::particle{
                {0.f, -15.f},
                0.f,
                3.f,
                block.color
            });
        };

        spawn_particle({-3.f, 3.f});
        spawn_particle({-3.f, -3.f});
        spawn_particle({3.f, -3.f});
        spawn_particle({3.f, 3.f});

        engine.entities.destroy_entity(eid);
    };


    engine.entities.visit([&](component::board& board) {
        auto check_matches = [&] {
            bool lines_broke = false;

            // Clear lines

            for (int y = 21; y >= 0; --y) {
                if (std::all_of(begin(board.grid[y]), end(board.grid[y]), [](auto& x) { return bool(x); })) {
                    lines_broke = true;
                    for (auto& nid : board.grid[y]) {
                        break_block(*nid);
                        nid = std::nullopt;
                    }
                    for (int oy = y + 1; oy < 22; ++oy) {
                        for (auto& nid : board.grid[oy]) {
                            if (nid) {
                                auto& pos = engine.entities.get_component<component::position>(engine.entities.get_entity(*nid));
                                pos.y -= 1;
                            }
                        }
                        board.grid[oy - 1] = std::move(board.grid[oy]);
                    }
                    board.grid[21] = {};
                }
            }

            // Clear color groups

            if (!lines_broke) {
                struct cell_info {
                    cell_info* root = nullptr;
                    int group_size = 1;
                };

                auto find_root = [](cell_info* cell) {
                    auto root = cell;
                    while (root->root) {
                        root = root->root;
                    }
                    if (root != cell) {
                        cell->root = root;
                    }
                    return root;
                };

                auto join_sets = [&find_root](cell_info* a, cell_info* b) {
                    a = find_root(a);
                    b = find_root(b);

                    if (a != b) {
                        if (b->group_size < a->group_size) {
                            std::swap(a, b);
                        }
                        a->root = b;
                        b->group_size += a->group_size;
                    }
                };

                std::array<std::array<cell_info, 10>, 22> cells;

                // Group em
                for (int y = 0; y < 22; ++y) {
                    for (int x = 0; x < 10; ++x) {
                        if (board.grid[y][x]) {
                            const auto& block = engine.entities.get_component<component::block>(engine.entities.get_entity(*board.grid[y][x]));
                            if (block.color != 0) {
                                if (y < 21 && board.grid[y + 1][x]) {
                                    const auto& other_block = engine.entities.get_component<component::block>(engine.entities.get_entity(*board.grid[y + 1][x]));
                                    if (other_block.color == block.color) {
                                        join_sets(&cells[y][x], &cells[y + 1][x]);
                                    }
                                }
                                if (x < 9 && board.grid[y][x + 1]) {
                                    const auto& other_block = engine.entities.get_component<component::block>(engine.entities.get_entity(*board.grid[y][x + 1]));
                                    if (other_block.color == block.color) {
                                        join_sets(&cells[y][x], &cells[y][x + 1]);
                                    }
                                }
                            }
                        }
                    }
                }

                // Break em
                for (int y = 21; y >= 0; --y) {
                    for (int x = 0; x < 10; ++x) {
                        if (find_root(&cells[y][x])->group_size >= 3) {
                            lines_broke = true;
                            break_block(*board.grid[y][x]);
                            for (int oy = y + 1; oy < 22; ++oy) {
                                if (board.grid[oy][x]) {
                                    auto& pos = engine.entities.get_component<component::position>(engine.entities.get_entity(*board.grid[oy][x]));
                                    pos.y -= 1;
                                }
                                board.grid[oy - 1][x] = std::move(board.grid[oy][x]);
                            }
                            board.grid[21][x] = std::nullopt;
                        }
                    }
                }
            }

            return lines_broke;
        };

        auto spawn_next = [&] {
            auto active = engine.entities.create_entity();
            engine.entities.create_component(active, component::position{4, 19});
            engine.entities.create_component(active, get_random_shape(engine.rng, engine.bag));
            board.active = engine.entities.get_component<component::net_id>(active).id;
        };

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
                        auto x = pos.x + shape.pieces[i].x;
                        auto y = pos.y + shape.pieces[i].y;
                        auto block = engine.entities.create_entity();
                        engine.entities.create_component(block, component::position{x, y});
                        engine.entities.create_component(block, component::block{shape.colors[i]});
                        board.grid[y][x] = engine.entities.get_component<component::net_id>(block).id;
                    }
                    engine.entities.destroy_entity(active);

                    board.active = std::nullopt;

                    if (!check_matches()) {
                        spawn_next();
                    }

                } else {
                    pos.y -= 1;
                }
            }
        } else {
            board.next_tick -= delta;

            // Chains
            if (board.next_tick <= 0.0) {
                board.next_tick += 0.25;
                if (!check_matches()) {
                    spawn_next();
                }
            }
        }
    });
}

}  //namespace systems
