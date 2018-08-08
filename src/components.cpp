#include "components.hpp"

#include "component_scripting.hpp"

namespace component {

void register_components(sol::table& component_table) {
    scripting::register_type<component::net_id>(component_table);
    scripting::register_type<component::position>(component_table);
    scripting::register_type<component::velocity>(component_table);
    scripting::register_type<component::aabb>(component_table);
    scripting::register_type<component::script>(component_table);
    scripting::register_type<component::animation>(component_table);
    scripting::register_type<component::death_timer>(component_table);
}

} //namespace compoennt
