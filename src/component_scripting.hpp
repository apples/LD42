#ifndef LD42_COMPONENT_SCRIPTING_HPP
#define LD42_COMPONENT_SCRIPTING_HPP

#include "json.hpp"
#include "scripting.hpp"
#include "entities.hpp"

#include <type_traits>
#include <utility>
#include <iostream>

namespace component {
namespace _detail {

template <typename T>
struct is_tag : std::false_type {};

template <typename T>
struct is_tag<ginseng::tag<T>> : std::true_type {};

template <typename T, typename... Args, std::enable_if_t<!is_tag<T>::value, int> = 0>
void new_component_usertype(sol::table& lua, const std::string& name, Args&&... args) {
    lua.new_usertype<T>(name,
        sol::constructors<T(), T(const T&)>{},
        std::forward<Args>(args)...,
        "_create_component", [=](ember_database& db, ember_database::ent_id eid, T com) -> ember_database::com_id {
            return db.create_component(eid, std::move(com));
        },
        "_destroy_component", [](ember_database& db, ember_database::ent_id eid) {
            db.destroy_component<T>(eid);
        },
        "_get_component", [=](ember_database& db, ember_database::ent_id eid) -> std::reference_wrapper<T> {
            if (!db.has_component<T>(eid)) {
                std::cerr << "ERROR: Attempting to get nonexistant component " << name << " from entity " << eid.get_index() << std::endl;
            }
            return std::ref(db.get_component<T>(eid));
        },
        "_has_component", [=](ember_database& db, ember_database::ent_id eid) {
            return db.has_component<T>(eid);
        });
}

template <typename T, typename... Args, std::enable_if_t<is_tag<T>::value, int> = 0>
void new_component_usertype(sol::table& lua, const std::string& name, Args&&... args) {
    lua.new_usertype<T>(name, std::forward<Args>(args)...,
        "_create_component", [=](ember_database& db, ember_database::ent_id eid, T com) {
            db.create_component(eid, std::move(com));
        },
        "_destroy_component", [](ember_database& db, ember_database::ent_id eid) {
            db.destroy_component<T>(eid);
        },
        "_has_component", [=](ember_database& db, ember_database::ent_id eid) {
            return db.has_component<T>(eid);
        });
}

} //namespace _detail

template <typename T, typename... Ts>
void register_usertype(scripting::token<T>, sol::table& lua, Ts&&... args) {
    _detail::new_component_usertype<T>(lua, meta::getName<T>(), std::forward<Ts>(args)...);
}

template <typename T>
void from_json(const nlohmann::json& json, T& msg) {
    using type = std::decay_t<T>;
    meta::doForAllMembers<type>([&](auto& member) {
            using member_type = meta::get_member_type<decltype(member)>;
            member.set(msg, json[member.getName()].template get<member_type>());
        });
}

template <typename T>
void to_json(nlohmann::json& json, const T& msg) {
    using type = std::decay_t<T>;
    meta::doForAllMembers<type>([&](auto& member) {
            json[member.getName()] = member.template get(msg);
        });
}

} //namespace component

#endif //LD42_COMPONENT_SCRIPTING_HPP
