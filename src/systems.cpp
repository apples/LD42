#include "systems.hpp"

#include "components.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <sushi/frustum.hpp>
#include <sushi/shader.hpp>
#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>
#include <sol.hpp>

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
        return
            a.left < b.right &&
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
    std::sort(begin(world), end(world), [](const entity_info& a, const entity_info& b){
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
    auto proj = glm::ortho(-7.5f * engine.aspect_ratio, 7.5f * engine.aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
    auto view = glm::mat4(1.f);
    auto frustum = sushi::frustum(proj * view);
    engine.entities.visit([&](DB::ent_id eid, const component::position& pos, component::animation& anim) {
        if (frustum.contains({pos.x, pos.y, 0.f}, std::sqrt(0.5 * 0.5 * 2.f))) {
            auto modelmat = glm::mat4(1);
            modelmat = glm::translate(modelmat, {int(pos.x * 16) / 16.f, int(pos.y * 16) / 16.f, 0});

            modelmat = glm::scale(modelmat, {anim.scale, anim.scale, anim.scale});
            modelmat = glm::rotate(modelmat, anim.rot, {0, 0, 1});
            modelmat = glm::translate(modelmat, {anim.offset_x, anim.offset_y, 0});

            auto tint = glm::vec4{1, 1, 1, 1};

            // animation code
            auto jsonAnim = *engine.resources.animation_cache.get(anim.name);
            auto tMilliSecond = float(jsonAnim[anim.cycle]["frame"][anim.frame]["t"]) / 1000.f;
            anim.t += delta / 10;
            if (anim.t > tMilliSecond) {
                int nextFrame = jsonAnim[anim.cycle]["frame"][anim.frame]["nextFrame"];
                anim.frame = nextFrame;
                anim.t = 0;
            }
            auto pathToTexture = jsonAnim[anim.cycle]["frame"][anim.frame]["path"];

            sushi::set_texture(0, *engine.resources.texture_cache.get(pathToTexture));
            sushi::set_uniform("normal_mat", glm::inverseTranspose(view * modelmat));
            sushi::set_uniform("MVP", (proj * view * modelmat));
            sushi::set_uniform("tint", tint);
            sushi::draw_mesh(engine.sprite_mesh);
        }
    });
}

}  //namespace systems
