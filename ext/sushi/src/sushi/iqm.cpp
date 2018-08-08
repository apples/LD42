//
// Created by Jeramy on 12/30/2015.
//

#include "iqm.hpp"

#include <algorithm>
#include <memory>
#include <cstdio>

namespace sushi {
namespace iqm {

iqm_data load_iqm(const std::string& fname) {
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> file (std::fopen(fname.c_str(), "rb"), &std::fclose);

    auto next_u8 = [&]{
        std::uint8_t rv = 0;
        int c = std::fgetc(file.get());
        if (c == EOF) {
            throw std::runtime_error("ERROR: " + fname + ": Unexpected EOF!");
        }
        rv |= std::uint8_t(c);
        return rv;
    };

    auto next_u32 = [&]{
        std::uint32_t rv = 0;
        for (int i=0; i<4; ++i) {
            auto c = next_u8();
            rv |= std::uint32_t(c) << (8 * i);
        }
        return rv;
    };

    auto next_u16 = [&]{
        std::uint16_t rv = 0;
        for (int i=0; i<2; ++i) {
            auto c = next_u8();
            rv |= std::uint16_t(c) << (8 * i);
        }
        return rv;
    };

    auto next_int = [&]{
        static_assert(sizeof(int)==sizeof(std::uint32_t), "Oh no.");
        auto i = next_u32();
        int rv;
        std::copy(reinterpret_cast<char*>(&i), reinterpret_cast<char*>(&i + 1), reinterpret_cast<char*>(&rv));
        return rv;
    };

    auto next_float = [&]{
        static_assert(sizeof(float)==sizeof(std::uint32_t), "NOOO!!!");
        auto i = next_u32();
        float rv;
        std::copy(reinterpret_cast<char*>(&i), reinterpret_cast<char*>(&i + 1), reinterpret_cast<char*>(&rv));
        return rv;
    };

    auto next_string = [&]{
        std::string rv;
        while (true) {
            int c = std::fgetc(file.get());
            if (c == EOF) {
                throw std::runtime_error("ERROR: " + fname + ": Unexpected EOF!");
            }
            if (char(c) == '\0') {
                return rv;
            }
            rv.push_back(char(c));
        }
    };

    auto magic = next_string();
    if (magic != "INTERQUAKEMODEL") {
        throw std::runtime_error("ERROR: " + fname + ": Bad magic string!");
    }

    auto version = next_u32();
    if (version != 2) {
        throw std::runtime_error("ERROR: " + fname + ": Wrong version!");
    }

    next_u32(); // filesize
    next_u32(); // flags
    auto num_text = next_u32();
    auto ofs_text = next_u32();
    auto num_meshes = next_u32();
    auto ofs_meshes = next_u32();
    auto num_vertexarrays = next_u32();
    auto num_vertexes = next_u32();
    auto ofs_vertexarrays = next_u32();
    auto num_triangles = next_u32();
    auto ofs_triangles = next_u32();
    auto ofs_adjacency = next_u32();
    auto num_joints = next_u32();
    auto ofs_joints = next_u32();
    auto num_poses = next_u32();
    auto ofs_poses = next_u32();
    auto num_anims = next_u32();
    auto ofs_anims = next_u32();
    auto num_frames = next_u32();
    auto num_framechannels = next_u32();
    auto ofs_frames = next_u32();
    auto ofs_bounds = next_u32();
    auto num_comment = next_u32();
    auto ofs_comment = next_u32();
    auto num_extensions = next_u32();
    auto ofs_extensions = next_u32();

    iqm_data rv;

    auto get_name = [&](long pos){
        auto cur = std::ftell(file.get());
        std::fseek(file.get(), ofs_text + pos, SEEK_SET);
        SUSHI_DEFER { std::fseek(file.get(), cur, SEEK_SET); };
        auto rv = next_string();
        return rv;
    };

    // text

    std::fseek(file.get(), ofs_text, SEEK_SET);
    for (auto i = 0u; i < num_text; ++i) {
        //rv.text.push_back(next_string()); // what exactly is this used for?
    }

    // meshes

    std::fseek(file.get(), ofs_meshes, SEEK_SET);
    for (auto i = 0u; i < num_meshes; ++i) {
        mesh m;
        m.name = get_name(next_u32());
        m.material = get_name(next_u32());
        m.first_vertex = next_u32();
        m.num_vertexes = next_u32();
        m.first_triangle = next_u32();
        m.num_triangles = next_u32();
        rv.meshes.push_back(std::move(m));
    }

    // vertexarrays

    std::fseek(file.get(), ofs_vertexarrays, SEEK_SET);
    for (auto i = 0u; i < num_vertexarrays; ++i) {
        auto type = next_u32();
        next_u32(); // flags
        auto format = next_u32();
        auto size = next_u32();
        auto offset = next_u32();

        auto cur = std::ftell(file.get());
        std::fseek(file.get(), offset, SEEK_SET);
        SUSHI_DEFER { std::fseek(file.get(), cur, SEEK_SET); };

        switch (type) {
            case 0: // position
                std::generate_n(std::back_inserter(rv.vertexarrays.position), num_vertexes * size, next_float);
                break;
            case 1: // texcoord
                std::generate_n(std::back_inserter(rv.vertexarrays.texcoord), num_vertexes * size, next_float);
                break;
            case 2: // normal
                std::generate_n(std::back_inserter(rv.vertexarrays.normal), num_vertexes * size, next_float);
                break;
            case 3: // tangent
                std::generate_n(std::back_inserter(rv.vertexarrays.tangent), num_vertexes * size, next_float);
                break;
            case 4: // blendindexes
                std::generate_n(std::back_inserter(rv.vertexarrays.blendindexes), num_vertexes * size, next_u8);
                break;
            case 5: // blendweights
                std::generate_n(std::back_inserter(rv.vertexarrays.blendweights), num_vertexes * size, next_u8);
                break;
            case 6: // color
                std::generate_n(std::back_inserter(rv.vertexarrays.color), num_vertexes * size, next_u8);
                break;
        }
    }

    // triangles

    std::fseek(file.get(), ofs_triangles, SEEK_SET);
    for (auto i = 0u; i < num_triangles; ++i) {
        triangle t;
        std::generate(std::begin(t.verts), std::end(t.verts), next_u32);
        rv.triangles.push_back(std::move(t));
    }

    // joints

    std::fseek(file.get(), ofs_joints, SEEK_SET);
    for (auto i = 0u; i < num_joints; ++i) {
        joint j;
        j.name = get_name(next_u32());
        j.parent = next_int();
        j.pos.x = next_float(); j.pos.y = next_float(); j.pos.z = next_float();
        j.rot.x = next_float(); j.rot.y = next_float(); j.rot.z = next_float(); j.rot.w = next_float();
        j.scl.x = next_float(); j.scl.y = next_float(); j.scl.z = next_float();
        rv.joints.push_back(std::move(j));
    }

    // poses

    std::fseek(file.get(), ofs_poses, SEEK_SET);
    for (auto i = 0u; i < num_poses; ++i) {
        pose p;
        p.parent = next_int();
        p.channels = next_u32();
        std::generate(std::begin(p.offsets), std::end(p.offsets), next_float);
        std::generate(std::begin(p.scales), std::end(p.scales), next_float);
        rv.poses.push_back(std::move(p));
    }

    // anims

    std::fseek(file.get(), ofs_anims, SEEK_SET);
    for (auto i = 0u; i < num_anims; ++i) {
        anim a;
        a.name = get_name(next_u32());
        a.first_frame = next_u32();
        a.num_frames = next_u32();
        a.framerate = next_float();
        auto flags = next_u32();
        a.loop = bool(flags & 1);
        rv.anims.push_back(std::move(a));
    }

    // frames

    std::fseek(file.get(), ofs_frames, SEEK_SET);
    std::generate_n(std::back_inserter(rv.frames), num_frames * num_framechannels, next_u16);
    rv.num_framechannels = num_framechannels;

    // bounds

    std::fseek(file.get(), ofs_bounds, SEEK_SET);
    for (auto i = 0u; i < num_frames; ++i) {
        bound b;
        b.min.x = next_float();
        b.min.y = next_float();
        b.min.z = next_float();
        b.max.x = next_float();
        b.max.y = next_float();
        b.max.z = next_float();
        b.xyradius = next_float();
        b.radius = next_float();
        rv.bounds.push_back(std::move(b));
    }

    // comments

    std::fseek(file.get(), ofs_comment, SEEK_SET);
    for (auto i = 0u; i < num_comment; ++i) {
        rv.comments.push_back(next_string());
    }

    // ignore extensions

    return rv;
}

} // namespace iqm
} // namespace sushi
