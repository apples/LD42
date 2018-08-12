#include "gameplay_state.hpp"

#include "systems.hpp"

#include <chrono>

void gameplay_state(ld42_engine& engine, double delta) {
    systems::movement(engine, delta);
    systems::collision(engine, delta);
    systems::scripting(engine, delta);
    systems::death_timer(engine, delta);
    systems::render(engine, delta);
    systems::board_tick(engine, delta);
}