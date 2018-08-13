#include "engine.hpp"

#include "components.hpp"
#include "component_scripting.hpp"
#include "sprite.hpp"
#include "utility.hpp"

#include "platform/platform.hpp"
#include "emberjs/config.hpp"
#include "sushi/sushi.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <unordered_map>

ld42_engine::ld42_engine() {
    std::cout << "Init..." << std::endl;
    
    running = true;

    std::cout << "Creating Lua state..." << std::endl;

    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    auto nlohmann_table = lua.create_named_table("component");
    nlohmann_table.new_usertype<nlohmann::json>("json");

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    component::register_components(component_table);

    input_table = lua.create_named_table("input");

    std::cout << "Initializing soloud..." << std::endl;

    soloud.init();

    std::cout << "Loading config..." << std::endl;

    config = emberjs::get_config();

    display_width = int(config["display"]["width"]);
    display_height = int(config["display"]["height"]);
    aspect_ratio = float(display_width) / float(display_height);

    std::cout << "Creating caches..." << std::endl;

    resources = resource_manager(config, lua);

    std::cout << "Setting helper functions..." << std::endl;

    lua["play_sfx"] = [&](const std::string& name){ play_sfx(name); };
    lua["play_music"] = [&](const std::string& name){ play_music(name); };
    lua["entity_from_json"] = [&](const nlohmann::json& json){ return entity_from_json(json); };

    std::cout << "Initializing SDL..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    std::cout << "Opening window..." << std::endl;

    g_window = SDL_CreateWindow("LD42 - Tetromatcher", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    std::cout << "Setting window attributes..." << std::endl;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    platform::set_gl_version();

    std::cout << "Creating GL context..." << std::endl;

    glcontext = SDL_GL_CreateContext(g_window);

    platform::load_gl_extensions();

    std::cout << "Loading shaders..." << std::endl;

    auto shader_path = platform::get_shader_path();

    program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, shader_path + "/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, shader_path + "/basic.frag"),
    });

    program_msdf = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, shader_path + "/msdf.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, shader_path + "/msdf.frag"),
    });

    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");

    sushi::set_program(program_msdf);
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::NORMAL, "normal");

    std::cout << "Loading common GPU objects..." << std::endl;

    framebuffer = sushi::create_framebuffer(utility::vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    framebuffer_mesh = make_sprite_mesh(framebuffer.color_texs[0]);

    sprite_mesh = sushi::load_static_mesh_data(
        {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
    );

    std::cout << "Initializing GUI..." << std::endl;

    renderer = sushi_renderer({320, 240}, program, program_msdf, resources.font_cache, resources.texture_cache);

    gui_screen = gui::screen({320, 240});
    gui_screen.show();

    root_widget = std::make_shared<gui::screen>(glm::vec2{320, 240});
    root_widget->hide();
    gui_screen.add_child(root_widget);

    score_stamp = std::make_shared<gui::label>();
    score_stamp->set_position({0,-1});
    score_stamp->set_font("LiberationSans-Regular");
    score_stamp->set_size(renderer, 10);
    score_stamp->set_text(renderer, "Score: 0");
    score_stamp->set_color({1,1,1,1});
    score_stamp->show();
    root_widget->add_child(score_stamp);

    lines_stamp = std::make_shared<gui::label>();
    lines_stamp->set_position({0,-25});
    lines_stamp->set_font("LiberationSans-Regular");
    lines_stamp->set_size(renderer, 10);
    lines_stamp->set_text(renderer, "Lines: 0");
    lines_stamp->set_color({1,1,1,1});
    lines_stamp->show();
    root_widget->add_child(lines_stamp);

    {
        auto restart_stamp = std::make_shared<gui::label>();
        restart_stamp->set_position({240, 24});
        restart_stamp->set_font("LiberationSans-Regular");
        restart_stamp->set_size(renderer, 8);
        restart_stamp->set_text(renderer, "R to restart");
        restart_stamp->set_color({1, 1, 1, 1});
        restart_stamp->show();
        root_widget->add_child(restart_stamp);
    }

    {
        auto music_stamp = std::make_shared<gui::label>();
        music_stamp->set_position({240, 8});
        music_stamp->set_font("LiberationSans-Regular");
        music_stamp->set_size(renderer, 8);
        music_stamp->set_text(renderer, "M to toggle music");
        music_stamp->set_color({1, 1, 1, 1});
        music_stamp->show();
        root_widget->add_child(music_stamp);
    }

    {
        auto panel = std::make_shared<gui::panel>();
        panel->set_texture("white");
        panel->set_size({2, 480});
        panel->set_position({88, 0});
        panel->show();
        root_widget->add_child(panel);
    }

    {
        auto panel = std::make_shared<gui::panel>();
        panel->set_texture("white");
        panel->set_size({2, 480});
        panel->set_position({210, 0});
        panel->show();
        root_widget->add_child(panel);
    }

    auto debug_root = std::make_shared<gui::screen>(glm::vec2{320, 240});
#ifndef NDEBUG
    debug_root->show();
#endif
    gui_screen.add_child(debug_root);

    framerate_stamp = std::make_shared<gui::label>();
    framerate_stamp->set_position({-1,-1});
    framerate_stamp->set_font("LiberationSans-Regular");
    framerate_stamp->set_size(renderer, 12);
    framerate_stamp->set_text(renderer, "");
    framerate_stamp->set_color({1,0,1,1});
    framerate_stamp->show();
    debug_root->add_child(framerate_stamp);

    std::cout << "Finalizing engine..." << std::endl;

    prev_time = clock::now();
    framerate_buffer.reserve(10);

    fade = 0.0;
    fade_dir = 1.0;

    rng = std::mt19937(std::random_device{}());

    music_on = false;
}

ld42_engine::~ld42_engine() {
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void ld42_engine::step(const std::function<void(ld42_engine& engine, double delta)>& step_func) {
    using namespace std::literals;

    const auto now = clock::now();
    const auto delta_time = now - prev_time;
    const auto delta = std::chrono::duration<double>(delta_time).count();

    prev_time = now;
    framerate_buffer.push_back(delta_time);

    if (framerate_buffer.size() >= 10) {
        const auto avg_frame_dur = std::accumulate(begin(framerate_buffer), end(framerate_buffer), 0ns) / framerate_buffer.size();
        const auto framerate = 1.0 / std::chrono::duration<double>(avg_frame_dur).count();

        framerate_stamp->set_text(renderer, std::to_string(std::lround(framerate)) + "fps");
        framerate_buffer.clear();
    }

    SDL_Event event[2];  // Array is needed to work around stack issue in SDL_PollEvent.
    while (SDL_PollEvent(&event[0])) {
        if (handle_gui_input(event[0])) break;
        if (handle_game_input(event[0])) break;
    }

    // Input
    {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        update_input(delta, "left", keys[SDL_SCANCODE_LEFT]);
        update_input(delta, "right", keys[SDL_SCANCODE_RIGHT]);
        update_input(delta, "up", keys[SDL_SCANCODE_UP]);
        update_input(delta, "down", keys[SDL_SCANCODE_DOWN]);
        update_input(delta, "shoot", keys[SDL_SCANCODE_SPACE]);
        update_input(delta, "rotate_cw", keys[SDL_SCANCODE_X] | keys[SDL_SCANCODE_F] | keys[SDL_SCANCODE_SPACE]);
        update_input(delta, "rotate_ccw", keys[SDL_SCANCODE_Z] | keys[SDL_SCANCODE_W] | keys[SDL_SCANCODE_Y] | keys[SDL_SCANCODE_Q]);
        update_input(delta, "restart", keys[SDL_SCANCODE_R]);
        update_input(delta, "music", keys[SDL_SCANCODE_M]);
    }

    // Music
    {
        if (input_table["music_pressed"]) {
            toggle_music("bgm");
        }
    }

    // Fade
    {
        fade = std::clamp(float(fade + delta * fade_dir), 0.f, 1.f);
    }

    score_stamp->set_text(renderer, "Score: " + std::to_string(score));
    lines_stamp->set_text(renderer, "Lines: " + std::to_string(lines_cleared));
    root_widget->hide();

    // Draw scene
    {
        sushi::set_framebuffer(framebuffer);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 320, 240);
        sushi::set_program(program);
        step_func(*this, delta);
    }

    // Draw screen
    {
        sushi::set_framebuffer(nullptr);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 640, 480);

        const auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
        const auto modelmat = glm::mat4(1.f);
        sushi::set_program(program);
        sushi::set_uniform("MVP", projmat * modelmat);
        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
        sushi::set_uniform("cam_forward", glm::vec3{0, 0, -1});
        sushi::set_uniform("s_texture", 0);
        sushi::set_uniform("tint", glm::vec4{fade, fade, fade, 1});
        sushi::set_texture(0, framebuffer.color_texs[0]);
        sushi::draw_mesh(framebuffer_mesh);

        renderer.begin();
        EMBER_DEFER { renderer.end(); };
        gui_screen.draw(renderer, {0, 0});
    }

    SDL_GL_SwapWindow(g_window);
    lua.collect_garbage();
}

bool ld42_engine::handle_game_input(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            std::cout << "Goodbye!" << std::endl;
            running = false;
            return true;
    }

    return false;
}

bool ld42_engine::handle_gui_input(SDL_Event& e) {
    switch (e.type) {
        case SDL_MOUSEBUTTONDOWN: {
            switch (e.button.button) {
                case SDL_BUTTON_LEFT: {
                    auto abs_click_pos = glm::vec2{e.button.x, display_height - e.button.y + 1};
                    auto widget_stack = get_descendent_stack(gui_screen, abs_click_pos - gui_screen.get_position());
                    while (!widget_stack.empty()) {
                        auto cur_widget = widget_stack.back();
                        auto widget_pos = get_absolute_position(*cur_widget);
                        auto rel_click_pos = abs_click_pos - widget_pos;
                        if (cur_widget->on_click) {
                            if (cur_widget->on_click(*cur_widget, rel_click_pos)) {
                                return true;
                            }
                        }
                        widget_stack.pop_back();
                    }
                    break;
                }
            }
            break;
        }
    }
    return false;
}

void ld42_engine::play_sfx(const std::string& name) {
    auto wav_ptr = resources.sfx_cache.get(name);
    soloud.stopAudioSource(*wav_ptr);
    soloud.play(*wav_ptr);
}

void ld42_engine::play_music(const std::string& name) {
    auto wav_ptr = resources.music_cache.get(name);
    soloud.stopAudioSource(*wav_ptr);
    soloud.play(*wav_ptr);
}

void ld42_engine::toggle_music(const std::string& name) {
    music_on = !music_on;
    auto wav_ptr = resources.music_cache.get(name);
    soloud.stopAudioSource(*wav_ptr);
    if (music_on) {
        soloud.play(*wav_ptr);
    }
}

ember_database::ent_id ld42_engine::entity_from_json(const nlohmann::json& json) {
    auto& loader = *resources.environment_cache.get("system/loader");
    auto data = json.get<std::unordered_map<std::string, nlohmann::json>>();
    return loader["load_entity"](json).get<ember_database::ent_id>();
}

void ld42_engine::update_input(double delta, const std::string& name, bool keystate) {
    if (input_table[name].valid()) {
        auto prev = bool(input_table[name]);
        auto curr = bool(keystate);
        input_table[name] = curr;
        input_table[name + "_pressed"] = curr && !prev;
        input_table[name + "_released"] = !curr && prev;
        input_table[name + "_repeat"] = false;

        if (curr) {
            auto timer = input_table.get_or(name + "_repeat_timer", 0.0);
            timer -= delta;
            if (timer <= 0.0) {
                timer += 0.1;
                input_table[name + "_repeat"] = true;
            }
            input_table[name + "_repeat_timer"] = timer;
        } else {
            input_table[name + "_repeat_timer"] = 0.0;
        }
    } else {
        input_table[name] = bool(keystate);
        input_table[name + "_pressed"] = bool(keystate);
        input_table[name + "_released"] = false;
        input_table[name + "_repeat"] = false;
        input_table[name + "_repeat_timer"] = 0.0;
    }
}

void ld42_engine::load_world(const nlohmann::json& json) {
    entities.visit([&](ember_database::ent_id eid) {
        entities.destroy_entity(eid);
    });
    auto& loader = *resources.environment_cache.get("system/loader");
    auto data = json.get<std::vector<std::unordered_map<std::string, nlohmann::json>>>();
    loader["load_world"](data);
}

double ld42_engine::get_tick_delay() {
    auto level = lines_cleared / 10;
    const int delay_table[30] = {
        48,
        43,
        38,
        33,
        28,
        23,
        18,
        13,
        8,
        6,
        5,
        5,
        5,
        4,
        4,
        4,
        3,
        3,
        3,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        1
    };

    return delay_table[std::min(level, 29)] / 60.0;
}