#include "components.hpp"

#include "component_scripting.hpp"

#include <optional>

// Partial specializations for std::optional
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json& j, const std::optional<T>& opt) {
            if (opt) {
                j = *opt;
            } else {
                j = nullptr;
            }
        }
        static void from_json(const json& j, std::optional<T>& opt) {
            if (j.is_null()) {
                opt = std::nullopt;
            } else {
                opt = j.get<T>();
            }
        }
    };
}

namespace component {

void register_components(sol::table& component_table) {
    scripting::register_type<component::net_id>(component_table);
    scripting::register_type<component::position>(component_table);
    scripting::register_type<component::velocity>(component_table);
    scripting::register_type<component::aabb>(component_table);
    scripting::register_type<component::script>(component_table);
    scripting::register_type<component::animation>(component_table);
    scripting::register_type<component::death_timer>(component_table);
    scripting::register_type<component::shape>(component_table);
    scripting::register_type<component::board>(component_table);
}

} //namespace compoennt
