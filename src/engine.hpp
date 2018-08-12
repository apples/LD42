#ifndef LD42_ENGINE_HPP
#define LD42_ENGINE_HPP

#include "components.hpp"
#include "entities.hpp"
#include "scripting.hpp"
#include "json.hpp"
#include "resources.hpp"
#include "sdl.hpp"
#include "gui.hpp"
#include "sushi_renderer.hpp"

#include <sushi/framebuffer.hpp>
#include <sushi/mesh.hpp>
#include <sushi/shader.hpp>

#include <soloud.h>

#include <functional>
#include <chrono>
#include <vector>
#include <string>
#include <random>

class ld42_engine {
public:
    ld42_engine();
    ld42_engine(const ld42_engine&) = delete;
    ld42_engine(ld42_engine&&) = delete;
    ld42_engine& operator=(const ld42_engine&) = delete;
    ld42_engine& operator=(ld42_engine&&) = delete;
    ~ld42_engine();

    void step(const std::function<void(ld42_engine& engine, double delta)>& step_func);

    bool handle_game_input(const SDL_Event& event);
    bool handle_gui_input(SDL_Event& event);

    void play_sfx(const std::string& name);
    void play_music(const std::string& name);
    ember_database::ent_id entity_from_json(const nlohmann::json& json);

    void update_input(const std::string& name, bool keystate);

    void load_world(const nlohmann::json& json);

    using clock = std::chrono::steady_clock;

    bool running;
    ember_database entities;
    sol::state lua;
    SoLoud::Soloud soloud;
    nlohmann::json config;
    int display_width;
    int display_height;
    float aspect_ratio;
    resource_manager resources;
    SDL_Window* g_window;
    SDL_GLContext glcontext;
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;
    sushi::unique_program program_msdf;
    sushi::static_mesh sprite_mesh;
    sol::table input_table;
    clock::time_point prev_time;
    std::vector<std::chrono::nanoseconds> framerate_buffer;
    sushi_renderer renderer;
    gui::screen gui_screen;
    std::shared_ptr<gui::screen> root_widget;
    std::shared_ptr<gui::label> framerate_stamp;
    double fade;
    double fade_dir;
    std::mt19937 rng;
    std::vector<component::shape> bag;
};

#endif // LD42_ENGINE_HPP
