#ifndef LD42_FONT_HPP
#define LD42_FONT_HPP

#include <sushi/mesh.hpp>
#include <sushi/shader.hpp>
#include <sushi/texture.hpp>

#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <string>
#include <unordered_map>
#include <memory>

struct FontDeleter {
    void operator()(msdfgen::FontHandle* ptr) {
        msdfgen::destroyFont(ptr);
    }
};

class msdf_font {
public:
    struct glyph {
        sushi::static_mesh mesh;
        sushi::texture_2d texture;
        float advance;
    };

    msdf_font() = default;
    msdf_font(const std::string& filename);

    const glyph& get_glyph(int unicode) const;

private:
    std::unique_ptr<msdfgen::FontHandle, FontDeleter> font;
    mutable std::unordered_map<int, glyph> glyphs;
};

#endif //LD42_FONT_HPP
