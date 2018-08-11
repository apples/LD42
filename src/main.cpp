
#include "sdl.hpp"
#include "emberjs/config.hpp"
#include "platform/platform.hpp"

#include "engine.hpp"
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

std::function<void()>* loop;
void main_loop(void* engine_ptr) try {
    auto& engine = *static_cast<ld42_engine*>(engine_ptr);
    engine.step(*loop);
} catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) try {
    auto engine = ld42_engine();

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

    engine.lua["set_game_state"] = set_game_state;

    std::cout << "Loading UI..." << std::endl;

    auto renderer = sushi_renderer({320, 240}, engine.program, engine.program_msdf, engine.resources.font_cache, engine.resources.texture_cache);

    auto main_menu_root_widget = gui::screen({320, 240});

    auto main_menu_bg = std::make_shared<gui::panel>();
    main_menu_bg->set_position({0,0});
    main_menu_bg->set_size({320,240});
    main_menu_bg->set_texture("bg/main_menu");
    main_menu_bg->show();

    main_menu_root_widget.add_child(main_menu_bg);

    auto framerate_stamp = std::make_shared<gui::label>();
    framerate_stamp->set_position({-1,-1});
    framerate_stamp->set_font("LiberationSans-Regular");
    framerate_stamp->set_size(renderer, 12);
    framerate_stamp->set_text(renderer, "");
    framerate_stamp->set_color({1,0,1,1});
    framerate_stamp->show();

    engine.root_widget.add_child(framerate_stamp);

    std::cout << "Preparing main loop..." << std::endl;

    using clock = std::chrono::steady_clock;
    auto prev_time = clock::now();
    auto framerate_buffer = std::vector<std::chrono::nanoseconds>();
    framerate_buffer.reserve(10);

    auto make_menu_state = [&](const std::string& name, const auto& on_next) {
        auto update_fade = [&engine](auto& fade, auto& fade_dir, auto delta, auto& on_next) {
            fade = std::min(float(fade + delta * fade_dir), 1.f);

            if (fade == 1.f && engine.input_table["shoot_pressed"]) {
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

            const auto keys = SDL_GetKeyboardState(nullptr);

            // Update

            engine.update_input("shoot", keys[SDL_SCANCODE_SPACE]);
            update_fade(fade, fade_dir, delta, on_next);

            // Render

            sushi::set_framebuffer(engine.framebuffer);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, 320, 240);

            auto proj = glm::ortho(-7.5f * engine.aspect_ratio, 7.5f * engine.aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
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
                sushi::set_program(engine.program);
                sushi::set_uniform("MVP", projmat * modelmat);
                sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                sushi::set_uniform("s_texture", 0);
                sushi::set_uniform("tint", glm::vec4{fade,fade,fade,1});
                sushi::set_texture(0, engine.framebuffer.color_texs[0]);
                sushi::draw_mesh(engine.framebuffer_mesh);
            }
        };
    };

    auto screen_fade = 0.0;
    auto screen_fade_dir = 1.0;

    main_menu_loop = make_menu_state(
        "main_menu", [&] {
            engine.entities.visit([&](ember_database::ent_id eid) {
                engine.entities.destroy_entity(eid);
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

        // Input

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        engine.update_input("left", SDL_SCANCODE_LEFT);
        engine.update_input("right", SDL_SCANCODE_RIGHT);
        engine.update_input("up", SDL_SCANCODE_UP);
        engine.update_input("down", SDL_SCANCODE_DOWN);
        engine.update_input("shoot", SDL_SCANCODE_SPACE);

        // Update

        systems::movement(engine.entities, delta);
        systems::collision(engine.entities, delta, engine.resources);
        systems::scripting(engine.entities, delta, engine.resources);
        systems::death_timer(engine.entities, delta, engine.resources);

        // Render

        sushi::set_framebuffer(engine.framebuffer);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 320, 240);

        auto proj = glm::ortho(-7.5f * engine.aspect_ratio, 7.5f * engine.aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
        auto view = glm::mat4(1.f);

        systems::render(engine.entities, delta, proj, view, engine.sprite_mesh, engine.resources);

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
            sushi::set_program(engine.program);
            sushi::set_uniform("MVP", projmat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("tint", glm::vec4{screen_fade,screen_fade,screen_fade,1});
            sushi::set_texture(0, engine.framebuffer.color_texs[0]);
            sushi::draw_mesh(engine.framebuffer_mesh);

            renderer.begin();
            engine.root_widget.draw(renderer, {0,0});
            renderer.end();
        }
    };

    std::cout << "Success." << std::endl;

    loop = &main_menu_loop;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &engine, 0, 1);
#else
    while (engine.running) main_loop(&engine);
#endif

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cout << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
