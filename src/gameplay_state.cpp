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

    systems::movement(engine.entities, delta);
    systems::collision(engine.entities, delta, engine.resources);
    systems::scripting(engine.entities, delta, engine.resources);
    systems::death_timer(engine.entities, delta, engine.resources);

    // Render

    auto proj = glm::ortho(-7.5f * engine.aspect_ratio, 7.5f * engine.aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
    auto view = glm::mat4(1.f);

    systems::render(engine.entities, delta, proj, view, engine.sprite_mesh, engine.resources);
}