#include "config.hpp"

#include "../utility.hpp"

using json = nlohmann::json;

namespace emberjs {

    nlohmann::json get_config() {
        auto config = ember_config_get();
        EMBER_DEFER { free(config); };
        return json::parse(config);
    }

} //namespace emberjs
