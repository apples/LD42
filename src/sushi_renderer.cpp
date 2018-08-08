#include "sushi_renderer.hpp"

#include <glm/gtc/matrix_inverse.hpp>

sushi_renderer::sushi_renderer(const glm::vec2& display_area, sushi::unique_program& program, sushi::unique_program& program_msdf, cache<msdf_font>& font_cache, cache<sushi::texture_2d>& texture_cache) :
    display_area(display_area),
    program(&program),
    program_msdf(&program_msdf),
    font_cache(&font_cache),
    texture_cache(&texture_cache)
{
    rectangle_mesh = sushi::load_static_mesh_data(
        {{0.f, 1.f, 0.f},{0.f, 0.f, 0.f},{1.f, 0.f, 0.f},{1.f, 1.f, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

void sushi_renderer::begin() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void sushi_renderer::end() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void sushi_renderer::draw_rectangle(const std::string& texture, glm::vec2 position, glm::vec2 size) {
    if (position.x < 0) position.x = float(display_area.x) + position.x + 1.f - size.x;
    if (position.y < 0) position.y = float(display_area.y) + position.y + 1.f - size.y;

    auto proj = glm::ortho(0.f, display_area.x, 0.f, display_area.y, -1.f, 1.f);
    auto model_mat = glm::mat4(1.f);
    model_mat = glm::translate(model_mat, glm::vec3(position, 0.f));
    model_mat = glm::scale(model_mat, glm::vec3(size, 1.f));

    sushi::set_program(*program);
    sushi::set_uniform("cam_forward", glm::vec3(0.0, 0.0, -1.0));
    sushi::set_texture(0, *texture_cache->get(texture));
    sushi::set_uniform("normal_mat", glm::inverseTranspose(model_mat));
    sushi::set_uniform("MVP", (proj*model_mat));
    sushi::draw_mesh(rectangle_mesh);
}

float sushi_renderer::get_text_width(const std::string& text, const std::string& fontname) {
    auto font = font_cache->get(fontname);
    float width = 0;
    for (auto c : text) {
        auto& glyph = font->get_glyph(c);
        width += glyph.advance;
    }
    return width;
}

void sushi_renderer::draw_text(const std::string& text, const std::string& fontname, const glm::vec4& color, glm::vec2 position, float size) {
    auto font = font_cache->get(fontname);
    auto proj = glm::ortho(0.f, display_area.x, 0.f, display_area.y, -1.f, 1.f);
    auto model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(position, 0.f)), glm::vec3{size, size, 1.f});

    sushi::set_program(*program_msdf);
    sushi::set_uniform("msdf", 0);
    sushi::set_uniform("pxRange", 4.f);
    sushi::set_uniform("fgColor", color);

    for (auto c : text) {
        auto& glyph = font->get_glyph(c);
        sushi::set_uniform("MVP", proj * model);
        sushi::set_uniform("texSize", glm::vec2{glyph.texture.width, glyph.texture.height});
        sushi::set_texture(0, glyph.texture);
        sushi::draw_mesh(glyph.mesh);
        model = glm::translate(model, glm::vec3{glyph.advance, 0.f, 0.f});
    }
}
