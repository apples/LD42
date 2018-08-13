#include "gameplay_state.hpp"

#include "systems.hpp"

#include <chrono>

auto gameplay_state(std::function<void(const std::string&)> set_state) -> std::function<void(ld42_engine&, double)> {
    return [set_state](ld42_engine& engine, double delta) {
        if (engine.input_table["restart"]) {
            set_state("main_menu");
            return;
        }

        systems::movement(engine, delta);
        systems::collision(engine, delta);
        systems::scripting(engine, delta);
        systems::death_timer(engine, delta);
        systems::render(engine, delta);
        systems::board_tick(engine, delta);

        engine.root_widget->show();
    };
}