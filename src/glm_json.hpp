#ifndef LD42_GLM_JSON_HPP
#define LD42_GLM_JSON_HPP

#include "json.hpp"

#include <glm/glm.hpp>

namespace glm {

inline void from_json(const nlohmann::json& json, glm::ivec2& vec) {
    vec.x = json["x"];
    vec.y = json["y"];
}

inline void to_json(nlohmann::json& json, const glm::ivec2& vec) {
    json["x"] = vec.x;
    json["y"] = vec.y;
}

inline void from_json(const nlohmann::json& json, glm::vec2& vec) {
    vec.x = json["x"];
    vec.y = json["y"];
}

inline void to_json(nlohmann::json& json, const glm::vec2& vec) {
    json["x"] = vec.x;
    json["y"] = vec.y;
}

} // namespace glm

#endif  //LD42_GLM_JSON_HPP
