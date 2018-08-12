#ifndef LD42_GLM_LUA_HPP
#define LD42_GLM_LUA_HPP

#include "scripting.hpp"

#include <glm/glm.hpp>

namespace scripting {

template <>
void register_type<glm::vec3>(sol::table& lua) {
    auto glm_table = lua["glm"];
    lua.new_usertype<glm::vec3>("vec3",
                                sol::constructors<
                                    glm::vec3(),
                                    glm::vec3(const glm::vec3&),
                                    glm::vec3(float, float, float)>{},
                                "x", &glm::vec3::x,
                                "y", &glm::vec3::y,
                                "z", &glm::vec3::z);
}

template <>
void register_type<glm::ivec2>(sol::table& lua) {
    auto glm_table = lua["glm"];
    lua.new_usertype<glm::ivec2>("ivec2",
                                 sol::constructors<
                                     glm::ivec2(),
                                     glm::ivec2(const glm::ivec2&),
                                     glm::ivec2(int, int)>{},
                                 "x", &glm::ivec2::x,
                                 "y", &glm::ivec2::y);
}

}  //namespace scripting

#endif  //LD42_GLM_LUA_HPP
