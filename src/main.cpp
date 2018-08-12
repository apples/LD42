
#include "emberjs/config.hpp"
#include "platform/platform.hpp"
#include "sdl.hpp"

#include "components.hpp"
#include "engine.hpp"
#include "font.hpp"
#include "gameplay_state.hpp"
#include "gui.hpp"
#include "resources.hpp"
#include "sushi_renderer.hpp"
#include "systems.hpp"
#include "utility.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/intersect.hpp>
#include <sushi/sushi.hpp>

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include <sol.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

using namespace std::literals;

std::function<void(ld42_engine& engine, double delta)> loop;
void main_loop(void* engine_ptr) try {
    auto& engine = *static_cast<ld42_engine*>(engine_ptr);
    engine.step(loop);
} catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) try {
    auto engine = ld42_engine();

    std::function<void(ld42_engine&, double)> main_menu_loop;
    std::function<void(ld42_engine&, double)> game_over_loop;
    std::function<void(ld42_engine&, double)> win_loop;

    auto set_game_state = [&](const std::string& name) {
        if (name == "main_menu")
            loop = main_menu_loop;
        else if (name == "gameplay")
            loop = gameplay_state;
        else if (name == "game_over")
            loop = game_over_loop;
        else if (name == "win")
            loop = win_loop;
        else
            std::cerr << "Invalid game state." << std::endl;
    };

    engine.lua["set_game_state"] = set_game_state;

    std::cout << "Preparing main loop..." << std::endl;

    auto make_menu_state = [&](const std::string& name, const auto& on_next) {
        auto bg = std::make_shared<gui::panel>();
        bg->set_position({0, 0});
        bg->set_size({320, 240});
        bg->set_texture("bg/" + name);
        bg->show();

        auto menu_screen = std::make_shared<gui::screen>(glm::vec2{320, 240});
        menu_screen->add_child(bg);

        return [menu_screen, on_next](ld42_engine& engine, double delta) {
            // Update

            const auto keys = SDL_GetKeyboardState(nullptr);
            engine.update_input("shoot", keys[SDL_SCANCODE_SPACE]);

            if (engine.fade == 1.f && engine.input_table["shoot_pressed"]) {
                engine.fade_dir = -1.f;
            } else if (engine.fade == 0.f) {
                engine.fade_dir = 1.f;
                on_next();
                return;
            }

            // Render

            engine.renderer.begin();
            EMBER_DEFER { engine.renderer.end(); };
            menu_screen->draw(engine.renderer, {0, 0});
        };
    };

    main_menu_loop = make_menu_state("main_menu", [&] {
        engine.entities.visit([&](ember_database::ent_id eid) {
            engine.entities.destroy_entity(eid);
        });
        engine.fade = 0.0;
        engine.fade_dir = 1.0;
        engine.load_world(nlohmann::json::array({}));

        auto active = engine.entities.create_entity();
        engine.entities.create_component(active, component::position{5, 5});
        engine.entities.create_component(active, component::shape{{{{0,0},{1,0},{1,1},{2,1}}}, {{1,0,0,1}}});
        {
            auto board = engine.entities.create_entity();
            engine.entities.create_component(board, component::board{
                {},
                engine.entities.get_component<component::net_id>(active).id
            });
        }
        
        set_game_state("gameplay");
    });

    game_over_loop = make_menu_state("game_over", [&] {
        set_game_state("main_menu");
    });

    win_loop = make_menu_state("win", [&] {
        set_game_state("main_menu");
    });

    std::cout << "Success." << std::endl;

    loop = main_menu_loop;

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
