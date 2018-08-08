
#include "sdl.hpp"
#include "emberjs/config.hpp"
#include "platform/platform.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "font.hpp"
#include "gui.hpp"
#include "resources.hpp"
#include "sushi_renderer.hpp"
#include "systems.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <sol.hpp>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>
#include <memory>
#include <random>

using namespace std::literals;

sushi::static_mesh sprite_mesh(const sushi::texture_2d& texture) {
    auto left = -texture.width / 2.f;
    auto right = texture.width / 2.f;
    auto bottom = -texture.height / 2.f;
    auto top = texture.height / 2.f;

    return sushi::load_static_mesh_data(
        {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

std::function<void()>* loop;
void main_loop() try {
    (*loop)();
} catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) try {
    std::cout << "Init..." << std::endl;

    bool running = true;

    ember_database entities;

    std::cout << "Creating Lua state..." << std::endl;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    auto nlohmann_table = lua.create_named_table("component");
    nlohmann_table.new_usertype<nlohmann::json>("json");

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    component::register_components(component_table);

    auto input_table = lua.create_named_table("input");
    auto update_input = [&](const std::string& name, bool keystate) {
        if (input_table[name].valid()) {
            auto prev = bool(input_table[name]);
            auto curr = bool(keystate);
            input_table[name] = curr;
            input_table[name + "_pressed"] = curr && !prev;
            input_table[name + "_released"] = !curr && prev;
        } else {
            input_table[name] = bool(keystate);
            input_table[name + "_pressed"] = bool(keystate);
            input_table[name + "_released"] = false;
        }
    };

    std::cout << "Initializing soloud..." << std::endl;

    SoLoud::Soloud soloud;
    soloud.init();

    std::cout << "Loading config..." << std::endl;

    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    std::cout << "Creating caches..." << std::endl;

    auto resources = resource_manager(config, lua);

    std::cout << "Setting helper functions..." << std::endl;

    std::function<void()> main_menu_loop;
    std::function<void()> gameplay_loop;
    std::function<void()> game_over_loop;
    std::function<void()> win_loop;

    auto set_game_state = [&](const std::string& name) {
        if (name == "main_menu") loop = &main_menu_loop;
        else if (name == "gameplay") loop = &gameplay_loop;
        else if (name == "game_over") loop = &game_over_loop;
        else if (name == "win") loop = &win_loop;
        else std::cerr << "Invalid game state." << std::endl;
    };

    lua["set_game_state"] = set_game_state;

    auto play_sfx = [&](const std::string& name) {
        auto wav_ptr = resources.sfx_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };

    auto play_music = [&](const std::string& name) {
        auto wav_ptr = resources.music_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };

    auto entity_from_json = [&](const nlohmann::json& json) {
        auto loader_ptr = resources.environment_cache.get("system/loader");
        auto eid = (*loader_ptr)["load_entity"](scripting::json_to_lua(lua, json)).get<ember_database::ent_id>();
        return eid;
    };

    lua["play_sfx"] = play_sfx;
    lua["play_music"] = play_music;
    lua["entity_from_json"] = entity_from_json;

    std::cout << "Initializing SDL..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    std::cout << "Opening window..." << std::endl;

    auto g_window = SDL_CreateWindow("LD41", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    std::cout << "Setting window attributes..." << std::endl;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    platform::set_gl_version();

    std::cout << "Creating GL context..." << std::endl;

    auto glcontext = SDL_GL_CreateContext(g_window);

    platform::load_gl_extensions();

    std::cout << "Loading shaders..." << std::endl;

    auto shader_path = platform::get_shader_path();

    auto program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, shader_path + "/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, shader_path + "/basic.frag"),
    });

    auto program_msdf = sushi::link_program({
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

    auto framebuffer = sushi::create_framebuffer(utility::vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    auto framebuffer_mesh = sprite_mesh(framebuffer.color_texs[0]);

    auto sprite_mesh = sushi::load_static_mesh_data(
        {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
    );

    std::cout << "Loading UI..." << std::endl;

    auto renderer = sushi_renderer({320, 240}, program, program_msdf, resources.font_cache, resources.texture_cache);

    auto main_menu_root_widget = gui::screen({320, 240});

    auto main_menu_bg = std::make_shared<gui::panel>();
    main_menu_bg->set_position({0,0});
    main_menu_bg->set_size({320,240});
    main_menu_bg->set_texture("bg/main_menu");
    main_menu_bg->show();

    main_menu_root_widget.add_child(main_menu_bg);

    auto root_widget = gui::screen({320, 240});

    root_widget.show();

    auto framerate_stamp = std::make_shared<gui::label>();
    framerate_stamp->set_position({-1,-1});
    framerate_stamp->set_font("LiberationSans-Regular");
    framerate_stamp->set_size(renderer, 12);
    framerate_stamp->set_text(renderer, "");
    framerate_stamp->set_color({1,0,1,1});
    framerate_stamp->show();

    root_widget.add_child(framerate_stamp);

    std::cout << "Preparing main loop..." << std::endl;

    auto handle_game_input = [&](const SDL_Event& event){
        switch (event.type) {
            case SDL_QUIT:
                std::cout << "Goodbye!" << std::endl;
                running = false;
                return true;
        }

        return false;
    };

    auto handle_gui_input = [&](SDL_Event& e){
        switch (e.type) {
            case SDL_MOUSEBUTTONDOWN: {
                switch (e.button.button) {
                    case SDL_BUTTON_LEFT: {
                        auto abs_click_pos = glm::vec2{e.button.x, display_height - e.button.y + 1};
                        auto widget_stack = get_descendent_stack(root_widget, abs_click_pos - root_widget.get_position());
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
    };

    using clock = std::chrono::steady_clock;
    auto prev_time = clock::now();
    auto framerate_buffer = std::vector<std::chrono::nanoseconds>();
    framerate_buffer.reserve(10);

    auto make_menu_state = [&](const std::string& name, const auto& on_next) {
        auto update_fade = [&input_table](auto& fade, auto& fade_dir, auto delta, auto& on_next) {
            fade = std::min(float(fade + delta * fade_dir), 1.f);

            if (fade == 1.f && input_table["shoot_pressed"]) {
                fade_dir = -1.f;
            }

            if (fade < 0.f) {
                fade = 0.f;
                fade_dir = 1.f;
                on_next();
                return;
            }
        };

        auto fade = 0.0;
        auto fade_dir = 1.0;

        return [&, name, on_next, fade, fade_dir, update_fade]() mutable {
            auto now = clock::now();
            auto delta_time = now - prev_time;
            prev_time = now;
            framerate_buffer.push_back(delta_time);

            if (framerate_buffer.size() >= 10) {
                auto avg_frame_dur = std::accumulate(begin(framerate_buffer), end(framerate_buffer), 0ns) / framerate_buffer.size();
                auto framerate = 1.0 / std::chrono::duration<double>(avg_frame_dur).count();

                framerate_stamp->set_text(renderer, std::to_string(std::lround(framerate)) + "fps");
                framerate_buffer.clear();
            }

            auto delta = std::chrono::duration<double>(delta_time).count();

            SDL_Event event[2]; // Array is needed to work around stack issue in SDL_PollEvent.
            while (SDL_PollEvent(&event[0]))
            {
                if (handle_gui_input(event[0])) break;
                if (handle_game_input(event[0])) break;
            }

            const auto keys = SDL_GetKeyboardState(nullptr);


            // Update

            update_input("shoot", keys[SDL_SCANCODE_SPACE]);
            update_fade(fade, fade_dir, delta, on_next);

            // Render

            sushi::set_framebuffer(framebuffer);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, 320, 240);

            auto proj = glm::ortho(-7.5f * aspect_ratio, 7.5f * aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
            auto view = glm::mat4(1.f);

            auto frustum = sushi::frustum(proj*view);

            main_menu_bg->set_texture("bg/"+name);
            renderer.begin();
            main_menu_root_widget.draw(renderer, {0,0});
            renderer.end();

            {
                sushi::set_framebuffer(nullptr);
                glClearColor(0,0,0,1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glViewport(0, 0, 640, 480);

                auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
                auto modelmat = glm::mat4(1.f);
                sushi::set_program(program);
                sushi::set_uniform("MVP", projmat * modelmat);
                sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                sushi::set_uniform("s_texture", 0);
                sushi::set_uniform("tint", glm::vec4{fade,fade,fade,1});
                sushi::set_texture(0, framebuffer.color_texs[0]);
                sushi::draw_mesh(framebuffer_mesh);
            }

            SDL_GL_SwapWindow(g_window);

            lua.collect_garbage();
        };
    };

    auto screen_fade = 0.0;
    auto screen_fade_dir = 1.0;

    main_menu_loop = make_menu_state(
        "main_menu", [&]{
            entities.visit([&](ember_database::ent_id eid) {
                    entities.destroy_entity(eid);
                });
            screen_fade = 0.0;
            screen_fade_dir = 1.0;
            set_game_state("gameplay");
        });

    game_over_loop = make_menu_state(
        "game_over", [&] {
            set_game_state("main_menu");
        });

    win_loop = make_menu_state(
        "win", [&] {
            set_game_state("main_menu");
        });

    gameplay_loop = [&]{
        auto now = clock::now();
        auto delta_time = now - prev_time;
        prev_time = now;
        framerate_buffer.push_back(delta_time);

        auto delta = std::chrono::duration<double>(delta_time).count();

        if (framerate_buffer.size() >= 10) {
            auto avg_frame_dur = std::accumulate(begin(framerate_buffer), end(framerate_buffer), 0ns) / framerate_buffer.size();
            auto framerate = 1.0 / std::chrono::duration<double>(avg_frame_dur).count();

            framerate_stamp->set_text(renderer, std::to_string(std::lround(framerate)) + "fps");
            framerate_buffer.clear();
        }

        // Event pump

        SDL_Event event[2]; // Array is needed to work around stack issue in SDL_PollEvent.
        while (SDL_PollEvent(&event[0])) {
            if (handle_gui_input(event[0])) break;
            if (handle_game_input(event[0])) break;
        }

        // Input

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        update_input("left", SDL_SCANCODE_LEFT);
        update_input("right", SDL_SCANCODE_RIGHT);
        update_input("up", SDL_SCANCODE_UP);
        update_input("down", SDL_SCANCODE_DOWN);
        update_input("shoot", SDL_SCANCODE_SPACE);

        // Update

        systems::movement(entities, delta);
        systems::collision(entities, delta, resources);
        systems::scripting(entities, delta, resources);
        systems::death_timer(entities, delta, resources);

        // Render

        sushi::set_framebuffer(framebuffer);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 320, 240);

        auto proj = glm::ortho(-7.5f * aspect_ratio, 7.5f * aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
        auto view = glm::mat4(1.f);

        systems::render(entities, delta, proj, view, sprite_mesh, resources);

        {
            sushi::set_framebuffer(nullptr);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, 640, 480);

            auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
            auto modelmat = glm::mat4(1.f);
            sushi::set_program(program);
            sushi::set_uniform("MVP", projmat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("tint", glm::vec4{screen_fade,screen_fade,screen_fade,1});
            sushi::set_texture(0, framebuffer.color_texs[0]);
            sushi::draw_mesh(framebuffer_mesh);

            renderer.begin();
            root_widget.draw(renderer, {0,0});
            renderer.end();
        }

        SDL_GL_SwapWindow(g_window);

        lua.collect_garbage();
    };

    std::cout << "Success." << std::endl;

    loop = &main_menu_loop;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (running) main_loop();
#endif

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cout << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
