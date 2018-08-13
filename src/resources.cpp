#include "resources.hpp"

#include "font.hpp"
#include "json.hpp"

#include <sushi/mesh.hpp>
#include <sushi/texture.hpp>

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include <sol.hpp>

resource_manager::resource_manager(nlohmann::json& config, sol::state& lua) :
    mesh_cache([](const std::string& name) {
        return sushi::load_static_mesh_file("data/models/" + name + ".obj");
    }),
    texture_cache([](const std::string& name) {
        return sushi::load_texture_2d("data/textures/" + name + ".png", false, false, true, false);
    }),
    animation_cache([](const std::string& name) {
        std::ifstream file("data/animations/" + name + ".json");
        nlohmann::json json;
        file >> json;
        return std::make_shared<nlohmann::json>(json);
    }),
    font_cache([](const std::string& fontname) {
        return msdf_font("data/fonts/" + fontname + ".ttf");
    }),
    environment_cache([&](const std::string& name) {
        auto env = sol::environment(lua, sol::create, lua.globals());
        lua.safe_script_file("data/scripts/" + name + ".lua", env);
        return env;
    }),
    sfx_cache([&](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sound/sfx/" + name + ".wav").c_str());
        return wav;
    }),
    music_cache([&](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sound/music/" + name + ".ogg").c_str());
        wav->setLooping(1);
        wav->setVolume(config["volume"]);
        return wav;
    })
{}
