#include "entities.hpp"

#include "components.hpp"

#include <iostream>

ember_database::ent_id ember_database::create_entity() {
    return create_entity(next_id++);
}

ember_database::ent_id ember_database::create_entity(ember_database::net_id id) {
    auto iter = netid_to_entid.find(id);

    if (iter != netid_to_entid.end()) {
        std::clog << "Warning: Entity " << id << " created twice!" << std::endl;
        return iter->second;
    }

    auto ent = database::create_entity();
    database::create_component(ent, component::net_id{id});
    netid_to_entid[id] = ent;

    return ent;
}

void ember_database::destroy_entity(ember_database::net_id id) {
    auto iter = netid_to_entid.find(id);

    if (iter == netid_to_entid.end()) {
        std::clog << "Warning: Attempted to erase unknown entity " << id << "!" << std::endl;
        return;
    }

    database::destroy_entity(iter->second);
    netid_to_entid.erase(iter);
}

ember_database::ent_id ember_database::get_entity(ember_database::net_id id) {
    return netid_to_entid.at(id);
}

ember_database::ent_id ember_database::get_or_create_entity(ember_database::net_id id) {
    auto iter = netid_to_entid.find(id);
    if (iter == netid_to_entid.end()) {
        return create_entity(id);
    } else {
        return iter->second;
    }
}

namespace scripting {

template <>
void register_type<ember_database>(sol::table& lua) {
    lua.new_usertype<ember_database::ent_id>("ent_id",
        sol::meta_function::to_string, [](const ember_database::ent_id& eid){
                                                 return std::to_string(eid.get_index()); });
    lua.new_usertype<ember_database>("ember_database",
        "create_entity", sol::overload(
            sol::resolve<ember_database::ent_id()>(&ember_database::create_entity),
            sol::resolve<ember_database::ent_id(ember_database::net_id)>(&ember_database::create_entity)),
        "destroy_entity", sol::resolve<void(ember_database::ent_id)>(&ember_database::destroy_entity),
        "get_entity", &ember_database::get_entity,
        "get_or_create_entity", &ember_database::get_or_create_entity,
        "exists", &ember_database::exists,
        "size", &ember_database::size,
        "create_component", [](ember_database& db, ember_database::ent_id eid, sol::userdata com){
            return com["_create_component"](db, eid, com);
        },
        "destroy_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type){
            return com_type["_destroy_component"](db, eid);
        },
        "get_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type){
            return com_type["_get_component"](db, eid);
        },
        "has_component", [](ember_database& db, ember_database::ent_id eid, sol::table com_type){
            return com_type["_has_component"](db, eid);
        });
}

} //namespace scripting
