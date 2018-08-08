#ifndef LD42_SCRIPTING_HPP
#define LD42_SCRIPTING_HPP

#include "json.hpp"

#include <Meta.h>
#include <sol.hpp>

#include <tuple>

namespace scripting {

template <typename T>
struct token {};

namespace detail {

template <typename... Ts>
struct types  {};

template <typename Ts, typename = void>
struct has_register : std::false_type {};

template <typename... Ts>
struct has_register<types<Ts...>, std::void_t<decltype(register_usertype(std::declval<Ts>()...))>> : std::true_type {};

template <typename T, typename... Ts>
std::enable_if_t<has_register<types<token<T>, sol::table&, Ts...>>::value> register_usertype_sfinae(sol::table& lua, Ts&&... args) {
    register_usertype(token<T>{}, lua, std::forward<Ts>(args)...);
}

template <typename T, typename... Ts>
std::enable_if_t<!has_register<types<token<T>, sol::table&, Ts...>>::value> register_usertype_sfinae(sol::table& lua, Ts&&... args) {
    lua.new_usertype<T>(meta::getName<T>(), std::forward<Ts>(args)...);
}

} //namespace _detail

template <typename T>
struct type {
    template <typename F, typename H, typename... Us>
    static void register_helper(const F& func, const H& head, const Us&... tail) {
        auto newfunc = [&](auto&&... args) {
            func(head.getName(), head.getPtr(), args...);
        };
        register_helper(newfunc, tail...);
    }

    template <typename F>
    static void register_helper(const F& func) {
        func();
    }

    static void call(sol::table& lua) {
        std::apply([&](auto&&... members) {
                register_helper([&](auto&&... args) {
                        detail::register_usertype_sfinae<T>(lua, args...);
                    }, members...);
            }, meta::getMembers<T>());
    }
};

template <typename T>
void register_type(sol::table& lua) {
    type<T>::call(lua);
}

auto json_to_lua(sol::state& lua, const nlohmann::json& json) -> sol::object;

} //namespace scripting

#endif //LD42_SCRIPTING_HPP
