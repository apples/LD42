#ifndef LD42_SUSHI_RENDERER_HPP
#define LD42_SUSHI_RENDERER_HPP

#include "gui.hpp"
#include "resource_cache.hpp"
#include "font.hpp"

#include <sushi/sushi.hpp>

#include <string>

class sushi_renderer : public gui::render_context {
public:
    template <typename T>
    using cache = resource_cache<T, std::string>;
    sushi_renderer(const glm::vec2& display_area, sushi::unique_program& program, sushi::unique_program& program_msdf, cache<msdf_font>& font_cache, cache<sushi::texture_2d>& texture_cache);

    virtual void begin() override;
    virtual void end() override;
    virtual void draw_rectangle(const std::string& texture, glm::vec2 position, glm::vec2 size) override;
    virtual void draw_text(const std::string& text, const std::string& font, const glm::vec4& color, glm::vec2 position, float size) override;
    virtual float get_text_width(const std::string& text, const std::string& font) override;

private:
    glm::vec2 display_area;
    sushi::unique_program* program;
    sushi::unique_program* program_msdf;
    cache<msdf_font>* font_cache;
    cache<sushi::texture_2d>* texture_cache;
    sushi::static_mesh rectangle_mesh;
};

#endif //LD42_SUSHI_RENDERER_HPP
