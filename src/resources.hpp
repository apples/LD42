#ifndef LD42_RESOURCES_HPP
#define LD42_RESOURCES_HPP

#include "resource_cache.hpp"
#include "json.hpp"
#include "font.hpp"

#include <sushi/mesh.hpp>
#include <sushi/texture.hpp>

#include <sol.hpp>

#include <soloud_wav.h>

class resource_manager {
public:
    resource_manager() = default;
    resource_manager(const resource_manager&) = delete;
    resource_manager(resource_manager&&) = default;
    resource_manager& operator=(const resource_manager&) = delete;
    resource_manager& operator=(resource_manager&&) = default;

    resource_manager(nlohmann::json& config, sol::state& lua);

    resource_cache<sushi::static_mesh> mesh_cache;
    resource_cache<sushi::texture_2d> texture_cache;
    resource_cache<nlohmann::json> animation_cache;
    resource_cache<msdf_font> font_cache;
    resource_cache<sol::environment> environment_cache;
    resource_cache<SoLoud::Wav> sfx_cache;
    resource_cache<SoLoud::Wav> music_cache;
};

#endif  // LD42_RESOURCES_HPP
