#include "gui.hpp"

#include "utility.hpp"

namespace gui {

glm::vec2 widget::get_size() const { return {0,0}; }

glm::vec2 widget::get_position() const { return position; }

void widget::set_position(const glm::vec2& pos) { position = pos; }

const std::vector<std::shared_ptr<widget>>& widget::get_children() const { return children; }

void widget::add_child(std::shared_ptr<widget> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

void widget::remove_child(widget* child) {
    for (auto& c : children) {
        if (c.get() == child) {
            std::swap(c, children.back());
            children.pop_back();
            return;
        }
    }
}

void widget::clear_children() {
    children.clear();
}

widget* widget::get_parent() const { return parent; }

bool widget::is_visible() const { return visible; }

void widget::show() { visible = true; }

void widget::hide() { visible = false; }

void widget::draw(render_context& renderer, glm::vec2 origin) const {
    draw_self(renderer, origin);
    origin += position;

    auto size = get_size();

    for (const auto& c : children) {
        if (c->is_visible()) {
            auto c_origin = origin;
            auto c_pos = c->get_position();
            auto c_size = c->get_size();

            if (c_pos.x < 0) { c_origin.x += size.x - c_size.x + 1; }
            if (c_pos.y < 0) { c_origin.y += size.y - c_size.y + 1; }

            c->draw(renderer, c_origin);
        }
    }
}

void widget::draw_self(render_context& renderer, glm::vec2 origin) const {}

glm::vec2 get_absolute_position(gui::widget& widget) {
    auto position = widget.get_position();
    auto size = widget.get_size();
    auto parent = widget.get_parent();
    if (parent) {
        auto parent_size = parent->get_size();
        if (position.x < 0) position.x = parent_size.x + position.x + 1.f - size.x;
        if (position.y < 0) position.y = parent_size.y + position.y + 1.f - size.y;
        auto parent_pos = get_absolute_position(*parent);
        position += parent_pos;
    }
    return position;
}

std::vector<gui::widget*> get_descendent_stack(gui::widget& widget, glm::vec2 position) {
    std::vector<gui::widget*> result;
    auto parent_pos = widget.get_position();
    auto parent_size = widget.get_size();
    for (auto&& child : widget.get_children()) {
        if (child->is_visible()) {
            auto child_pos = child->get_position();
            auto child_size = child->get_size();
            if (child_pos.x < 0) child_pos.x = parent_size.x + child_pos.x + 1.f - child_size.x;
            if (child_pos.y < 0) child_pos.y = parent_size.y + child_pos.y + 1.f - child_size.y;
            if (position.x >= child_pos.x && position.x < child_pos.x + child_size.x && position.y >= child_pos.y && position.y < child_pos.y + child_size.y) {
                result.push_back(child.get());
                auto child_stack = get_descendent_stack(*child, position - child_pos);
                result.insert(end(result), make_move_iterator(begin(child_stack)), make_move_iterator(end(child_stack)));
            }
        }
    }
    return result;
}

screen::screen(const glm::vec2& screen_size) : size(screen_size) {}

glm::vec2 screen::get_size() const { return size; }

const std::string& label::get_text() const { return text; }

void label::set_text(render_context& renderer, const std::string& s) {
    text = s;
    size.x = size.y * renderer.get_text_width(text, font);
}

const std::string& label::get_font() const { return font; }

void label::set_font(const std::string& s) { font = s; }

const glm::vec4& label::get_color() const { return color; }

void label::set_color(const glm::vec4& c) { color = c; }

glm::vec2 label::get_size() const { return size; }

void label::set_size(render_context& renderer, float height) {
    size.y = height;
    size.x = size.y * renderer.get_text_width(text, font);
}

void label::draw_self(render_context& renderer, glm::vec2 origin) const {
    renderer.draw_text(text, font, color, origin + get_position(), size.y);
}

const std::string& panel::get_texture() const { return texture; }

void panel::set_texture(const std::string& tex) { texture = tex; }

glm::vec2 panel::get_size() const { return size; }

void panel::set_size(const glm::vec2& sz) { size = sz; }

void panel::draw_self(render_context& renderer, glm::vec2 origin) const {
    renderer.draw_rectangle(texture, origin + get_position(), get_size());
}

} //namespace gui
