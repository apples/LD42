#ifndef CLIENT_EMBERJS_CONFIG_HPP
#define CLIENT_EMBERJS_CONFIG_HPP

#include "../json.hpp"

namespace emberjs {

    extern "C" {
        extern char* ember_config_get();
    }

    nlohmann::json get_config();

} //namespace emberjs

#endif //CLIENT_EMBERJS_CONFIG_HPP
