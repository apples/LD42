#ifndef LD42_GLM_LUA_HPP
#define LD42_GLM_LUA_HPP

#include "scripting.hpp"

namespace scripting {

template <>
void register_type<glm::vec3>(sol::table& lua) {
    auto glm_table = lua["glm"];
    lua.new_usertype<glm::vec3>("vec3",
                                sol::constructors<
                                glm::vec3(),
                                glm::vec3(const glm::vec3&),
                                glm::vec3(float,float,float)>{});
}

} //namespace scripting


#endif //LD42_GLM_LUA_HPP
