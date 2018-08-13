#ifndef LD42_GAMEPLAY_STATE
#define LD42_GAMEPLAY_STATE

#include "engine.hpp"

#include <functional>

auto gameplay_state(std::function<void(const std::string&)> set_state) -> std::function<void(ld42_engine&, double)>;

#endif // LD42_GAMEPLAY_STATE
