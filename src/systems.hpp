#ifndef LD42_SYSTEMS_HPP
#define LD42_SYSTEMS_HPP

#include "entities.hpp"
#include "resources.hpp"
#include "json.hpp"

#include <glm/glm.hpp>
#include <sol.hpp>
#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>

namespace systems {

using DB = ember_database;

void movement(DB& entities, double delta);
void collision(DB& entities, double delta, resource_manager& resources);
void scripting(DB& entities, double delta, resource_manager& resources);
void death_timer(DB& entities, double delta, resource_manager& resources);
void render(DB& entities, double delta, glm::mat4 proj, glm::mat4 view, sushi::static_mesh& sprite_mesh, resource_manager& resources);

} //namespace systems

#endif //LD42_SYSTEMS_HPP
