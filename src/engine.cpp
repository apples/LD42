#include "engine.hpp"

#include "components.hpp"
#include "sprite.hpp"

#include "platform/platform.hpp"
#include "emberjs/config.hpp"

#include <iostream>

ld42_engine::ld42_engine() {
    std::cout << "Init..." << std::endl;
    
    running = true;

    std::cout << "Creating Lua state..." << std::endl;

    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    auto nlohmann_table = lua.create_named_table("component");
    nlohmann_table.new_usertype<nlohmann::json>("json");

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    component::register_components(component_table);

    input_table = lua.create_named_table("input");

    std::cout << "Initializing soloud..." << std::endl;

    soloud.init();

    std::cout << "Loading config..." << std::endl;

    config = emberjs::get_config();

    display_width = int(config["display"]["width"]);
    display_height = int(config["display"]["height"]);
    aspect_ratio = float(display_width) / float(display_height);

    std::cout << "Creating caches..." << std::endl;

    resources = resource_manager(config, lua);

    std::cout << "Setting helper functions..." << std::endl;

    lua["play_sfx"] = [&](const std::string& name){ play_sfx(name); };
    lua["play_music"] = [&](const std::string& name){ play_music(name); };
    lua["entity_from_json"] = [&](const nlohmann::json& json){ return entity_from_json(json); };

    std::cout << "Initializing SDL..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    std::cout << "Opening window..." << std::endl;

    g_window = SDL_CreateWindow("LD41", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    std::cout << "Setting window attributes..." << std::endl;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    platform::set_gl_version();

    std::cout << "Creating GL context..." << std::endl;

    glcontext = SDL_GL_CreateContext(g_window);

    platform::load_gl_extensions();

    std::cout << "Loading shaders..." << std::endl;

    auto shader_path = platform::get_shader_path();

    program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, shader_path + "/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, shader_path + "/basic.frag"),
    });

    program_msdf = sushi::link_program({
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

    framebuffer = sushi::create_framebuffer(utility::vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    framebuffer_mesh = make_sprite_mesh(framebuffer.color_texs[0]);

    sprite_mesh = sushi::load_static_mesh_data(
        {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
    );

    root_widget = gui::screen({320, 240});
    root_widget.show();
}

ld42_engine::~ld42_engine() {
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void ld42_engine::step(const std::function<void()>& step_func) {
    SDL_Event event[2];  // Array is needed to work around stack issue in SDL_PollEvent.
    while (SDL_PollEvent(&event[0])) {
        if (handle_gui_input(event[0])) break;
        if (handle_game_input(event[0])) break;
    }

    step_func();
    
    SDL_GL_SwapWindow(g_window);
    lua.collect_garbage();
}

bool ld42_engine::handle_game_input(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            std::cout << "Goodbye!" << std::endl;
            running = false;
            return true;
    }

    return false;
}

bool ld42_engine::handle_gui_input(SDL_Event& e) {
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
}

void ld42_engine::play_sfx(const std::string& name) {
    auto wav_ptr = resources.sfx_cache.get(name);
    soloud.stopAudioSource(*wav_ptr);
    soloud.play(*wav_ptr);
}

void ld42_engine::play_music(const std::string& name) {
    auto wav_ptr = resources.music_cache.get(name);
    soloud.stopAudioSource(*wav_ptr);
    soloud.play(*wav_ptr);
}

ember_database::ent_id ld42_engine::entity_from_json(const nlohmann::json& json) {
    auto loader_ptr = resources.environment_cache.get("system/loader");
    auto eid = (*loader_ptr)["load_entity"](scripting::json_to_lua(lua, json)).get<ember_database::ent_id>();
    return eid;
}

void ld42_engine::update_input(const std::string& name, bool keystate) {
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
}