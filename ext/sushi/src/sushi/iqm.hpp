//
// Created by Jeramy on 12/30/2015.
//

#ifndef SUSHI_IQM_HPP
#define SUSHI_IQM_HPP

#include "common.hpp"
#include "gl.hpp"

#include <string>
#include <bitset>
#include <cstdint>
#include <vector>

namespace sushi {
namespace iqm {

struct mesh {
    std::string name;
    std::string material;
    std::uint32_t first_vertex;
    std::uint32_t num_vertexes;
    std::uint32_t first_triangle;
    std::uint32_t num_triangles;
};

struct triangle {
    int verts[3];
};

struct joint {
    std::string name;
    int parent;
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scl;
};

struct pose {
    int parent;
    std::bitset<10> channels;
    float offsets[10];
    float scales[10];
};

struct anim {
    std::string name;
    std::uint32_t first_frame;
    std::uint32_t num_frames;
    float framerate;
    bool loop;
};

struct bound {
    glm::vec3 min;
    glm::vec3 max;
    float xyradius;
    float radius;
};

struct iqm_data {
    std::vector<std::string> text;
    std::vector<mesh> meshes;
    struct {
        std::vector<float> position;
        std::vector<float> texcoord;
        std::vector<float> normal;
        std::vector<float> tangent;
        std::vector<std::uint8_t> blendindexes;
        std::vector<std::uint8_t> blendweights;
        std::vector<std::uint8_t> color;
    } vertexarrays;
    std::vector<triangle> triangles;
    std::vector<joint> joints;
    std::vector<pose> poses;
    std::vector<anim> anims;
    std::vector<std::uint16_t> frames;
    std::uint32_t num_framechannels;
    std::vector<bound> bounds;
    std::vector<std::string> comments;
};

/// Loads an IQM file.
/// See http://sauerbraten.org/iqm/ for details.
/// \param fname Name of file.
/// \return IQM data.
iqm_data load_iqm(const std::string& fname);

} // namespace iqm
} // namespace sushi

#endif //SUSHI_IQM_HPP
