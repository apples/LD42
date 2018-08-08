#ifndef LD42_ENTITIES_HPP
#define LD42_ENTITIES_HPP

#include "scripting.hpp"
#include "json.hpp"
#include "utility.hpp"

#include <ginseng/ginseng.hpp>

#include <Meta.h>

#include <cstdint>
#include <unordered_map>

class ember_database : public ginseng::database {
    template <typename... Coms>
    struct entity_serializer {
        static nlohmann::json serialize(ember_database& db, ent_id eid) {
            return nlohmann::json::object();
        }
    };

    template <typename Head, typename... Coms>
    struct entity_serializer<Head, Coms...> {
        static nlohmann::json serialize(ember_database& db, ent_id eid) {
            auto json = entity_serializer<Coms...>::serialize(db, eid);
            if (db.has_component<Head>(eid)) {
                auto name = meta::getName<Head>();
                json[name] = db.get_component<Head>(eid);
            }
            return json;
        }
    };

public:
    using net_id = std::int64_t;

    using ginseng::database::destroy_entity;

    ent_id create_entity();

    ent_id create_entity(net_id id);

    void destroy_entity(net_id id);

    ent_id get_entity(net_id id);

    ent_id get_or_create_entity(net_id id);

    template <typename... Coms>
    nlohmann::json serialize_entity(ent_id eid) {
        return entity_serializer<Coms...>::serialize(*this, eid);
    }

private:
    net_id next_id = 1;
    std::unordered_map<net_id, ent_id> netid_to_entid;
};

namespace scripting {

template <>
void register_type<ember_database>(sol::table& lua);

} //namespace scripting

#endif //LD42_ENTITIES_HPP
