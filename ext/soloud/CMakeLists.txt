cmake_minimum_required (VERSION 2.6)
project(SoLoud)

file(GLOB_RECURSE soloud_SOURCES src/audiosource/* src/filter/* src/core/*)
file(GLOB_RECURSE soloud_PLATFORM_SOURCES src/backend/sdl2_static/*)

add_library(soloud ${soloud_SOURCES} ${soloud_PLATFORM_SOURCES})
target_include_directories(soloud PUBLIC include)
target_compile_definitions(soloud PUBLIC "WITH_SDL2_STATIC")

if (EMSCRIPTEN)
    set_target_properties(soloud PROPERTIES
        COMPILE_FLAGS "-s USE_SDL=2"
        LINK_FLAGS "-s USE_SDL=2")
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(SDL2 REQUIRED sdl2)

    target_link_libraries(soloud ${SDL2_LIBRARIES})
    target_include_directories(soloud PUBLIC ${SDL2_INCLUDE_DIRS})
endif()
