#include "font.hpp"

#include "json.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

msdfgen::FreetypeHandle* font_init() {
    static const auto ft = msdfgen::initializeFreetype();
    return ft;
}

} //static

msdf_font::msdf_font(const std::string& fontname) {
    auto ft = font_init();

    if (!ft) {
        throw std::runtime_error("Failed to initialize FreeType.");
    }

    font = decltype(font)(msdfgen::loadFont(ft, fontname.c_str()));

    if (!font) {
        throw std::runtime_error("Failed to load font "+fontname+".");
    }
}

const msdf_font::glyph& msdf_font::get_glyph(int unicode) const {
    auto iter = glyphs.find(unicode);

    if (iter == end(glyphs)) {
        msdfgen::Shape shape;
        double advance;

        if (!msdfgen::loadGlyph(shape, font.get(), unicode, &advance)) {
            return glyphs.at(0);
        }

        shape.normalize();
        msdfgen::edgeColoringSimple(shape, 3.0);

        double left=0, bottom=0, right=0, top=0;
        shape.bounds(left, bottom, right, top);

        left -= 1;
        bottom -= 1;
        right += 1;
        top += 1;

        auto width = int(right - left + 1);
        auto height = int(top - bottom + 1);

        msdfgen::Bitmap<msdfgen::FloatRGB> msdf(width, height);
        msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(-left, -bottom));

        std::vector<unsigned char> pixels;
        pixels.reserve(4*msdf.width()*msdf.height());
        for (int y = 0; y < msdf.height(); ++y) {
            for (int x = 0; x < msdf.width(); ++x) {
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).r*0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).g*0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).b*0x100), 0xff));
                pixels.push_back(255);
            }
        }

        double em;
        msdfgen::getFontScale(em, font.get());

        left /= em;
        right /= em;
        bottom /= em;
        top /= em;
        advance /= em;

        auto g = glyph{};

        g.mesh = sushi::load_static_mesh_data(
            {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
            {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
            {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
            {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});

        g.texture.handle = sushi::make_unique_texture();
        g.texture.width = width;
        g.texture.height = height;

        glBindTexture(GL_TEXTURE_2D, g.texture.handle.get());
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, msdf.width(), msdf.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

        g.advance = advance;

        iter = glyphs.insert({unicode, std::move(g)}).first;
    }

    return iter->second;
}

//msdf_font::msdf_font(const std::string& fontname) {
//    auto fontpath = "data/fonts/" + fontname;
//    auto fontfilename = fontpath + "/" + fontname + ".json";

//    std::ifstream fontfile (fontfilename);
//    nlohmann::json json;
//    fontfile >> json;

//    auto max_top = float(json["max_top"]);

//    std::cout << "max_top: " << max_top << std::endl;

//    auto&& glyphs_json = json["glyphs"];

//    for (auto iter = begin(glyphs_json); iter != end(glyphs_json); ++iter) {
//        auto unicode = std::stoi(iter.key());
//        auto details = iter.value();

//        auto width = int(details["width"]);
//        auto height = int(details["height"]);
//        auto left = float(details["left"]);
//        auto bottom = float(details["bottom"]);
//        auto advance = float(details["advance"]);
//        auto filename = fontpath + "/" + details["filename"].get<std::string>();

//        auto right = left + width;
//        auto top = bottom + height;

//        auto& g = glyphs[unicode];

//        left /= max_top;
//        right /= max_top;
//        bottom /= max_top;
//        top /= max_top;
//        advance /= max_top;

//        g.mesh = sushi::load_static_mesh_data(
//            {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
//            {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
//            {{0.f, 1.f},{0.f, 0.f},{1.f, 0.f},{1.f, 1.f}},
//            {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
//        );

//        g.texture = sushi::load_texture_2d(filename, true, false, false, false);

//        g.advance = advance;
//    }
//}

//const msdf_font::glyph& msdf_font::get_glyph(int unicode) const {
//    auto iter = glyphs.find(unicode);
//    if (iter != end(glyphs)) {
//        return iter->second;
//    } else {
//        return glyphs.at(0);
//    }
//}
