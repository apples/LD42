#include "scripting.hpp"

#include "json.hpp"

namespace scripting {

auto json_to_lua(sol::state& lua, const nlohmann::json& json) -> sol::object {
    using value_t = nlohmann::json::value_t;
    switch (json.type())
    {
    case value_t::null:
        return sol::make_object(lua, sol::nil);
    case value_t::object:
    {
        auto obj = lua.create_table();
        for (auto it = json.begin(); it != json.end(); ++it)
        {
            obj[it.key()] = json_to_lua(lua, it.value());
        }
        return obj;
    }
    case value_t::array:
    {
        auto obj = lua.create_table();
        for (auto i = 0; i < json.size(); ++i)
        {
            obj[i + 1] = json_to_lua(lua, json[i]);
        }
        return obj;
    }
    case value_t::string:
        return sol::make_object(lua, json.get<std::string>());
    case value_t::boolean:
        return sol::make_object(lua, json.get<bool>());
    case value_t::number_integer:
        return sol::make_object(lua, json.get<int>());
    case value_t::number_unsigned:
        return sol::make_object(lua, json.get<unsigned>());
    case value_t::number_float:
        return sol::make_object(lua, json.get<double>());
    default:
        return sol::make_object(lua, sol::nil);
    }
}

} //namespace scripting
