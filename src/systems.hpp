#ifndef LD42_SYSTEMS_HPP
#define LD42_SYSTEMS_HPP

#include "engine.hpp"

namespace systems {

void movement(ld42_engine& engine, double delta);
void collision(ld42_engine& engine, double delta);
void scripting(ld42_engine& engine, double delta);
void death_timer(ld42_engine& engine, double delta);
void render(ld42_engine& engine, double delta);
void board_tick(ld42_engine& engine, double delta);

} //namespace systems

#endif //LD42_SYSTEMS_HPP
