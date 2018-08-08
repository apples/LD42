#ifndef LD42_GUI_HPP
#define LD42_GUI_HPP

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace gui {

class render_context {
public:
    virtual ~render_context() = 0;

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void draw_rectangle(const std::string& texture, glm::vec2 position, glm::vec2 size) = 0;
    virtual void draw_text(const std::string& text, const std::string& font, const glm::vec4& color, glm::vec2 position, float size) = 0;
    virtual float get_text_width(const std::string& text, const std::string& font) = 0;
};

inline render_context::~render_context() = default;

class widget {
public:
    widget() = default;
    virtual ~widget() = 0;

    virtual glm::vec2 get_size() const;

    glm::vec2 get_position() const;
    void set_position(const glm::vec2& pos);

    const std::vector<std::shared_ptr<widget>>& get_children() const;
    void add_child(std::shared_ptr<widget> child);
    void remove_child(widget* child);
    void clear_children();

    widget* get_parent() const;

    bool is_visible() const;
    void show();
    void hide();

    void draw(render_context& renderer, glm::vec2 origin) const;
    virtual void draw_self(render_context& renderer, glm::vec2 origin) const;

    std::function<bool(widget& self, glm::vec2 click_pos)> on_click;

private:
    glm::vec2 position = {0,0};
    std::vector<std::shared_ptr<widget>> children = {};
    widget* parent = nullptr;
    bool visible = false;
};

inline widget::~widget() = default;

glm::vec2 get_absolute_position(gui::widget& widget);

std::vector<gui::widget*> get_descendent_stack(gui::widget& widget, glm::vec2 position);

class screen : public widget {
public:
    screen(const glm::vec2& screen_size);

    virtual glm::vec2 get_size() const override;

private:
    glm::vec2 size;
};

class label : public widget {
public:
    const std::string& get_text() const;
    void set_text(render_context& renderer, const std::string& s);

    const std::string& get_font() const;
    void set_font(const std::string& s);

    const glm::vec4& get_color() const;
    void set_color(const glm::vec4& c);

    virtual glm::vec2 get_size() const override final;
    void set_size(render_context& renderer, float height);

    virtual void draw_self(render_context& renderer, glm::vec2 origin) const override;

private:
    std::string text;
    std::string font;
    glm::vec4 color;
    glm::vec2 size;
};

class panel : public widget {
public:
    const std::string& get_texture() const;
    void set_texture(const std::string& tex);

    virtual glm::vec2 get_size() const override;
    void set_size(const glm::vec2& sz);

    virtual void draw_self(render_context& renderer, glm::vec2 origin) const override;

private:
    std::string texture;
    glm::vec2 size = {0,0};
};

} //namespace gui

#endif //LD42_GUI_HPP
