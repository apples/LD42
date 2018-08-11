#include "gameplay_state.hpp"

#include "systems.hpp"

#include <chrono>

void gameplay_state(ld42_engine& engine, double delta) {
    // Input

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    engine.update_input("left", SDL_SCANCODE_LEFT);
    engine.update_input("right", SDL_SCANCODE_RIGHT);
    engine.update_input("up", SDL_SCANCODE_UP);
    engine.update_input("down", SDL_SCANCODE_DOWN);
    engine.update_input("shoot", SDL_SCANCODE_SPACE);

    // Update

    systems::movement(engine, delta);
    systems::collision(engine, delta);
    systems::scripting(engine, delta);
    systems::death_timer(engine, delta);
    systems::render(engine, delta);
}