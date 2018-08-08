//
// Created by Jeramy on 8/22/2015.
//

#include "mesh.hpp"

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

namespace sushi {

namespace {

static_mesh upload_static_mesh(const std::vector<GLfloat>& data, int num_tris) {
    static_mesh rv;

    rv.vao = make_unique_vertex_array();
    rv.vertex_buffer = make_unique_buffer();
    rv.num_triangles = num_tris;
    rv.bounding_sphere = 0;

    for (int i=0; i<num_tris*3; ++i) {
        auto pos = glm::vec3{data[i*8+0],data[i*8+1],data[i*8+2]};
        rv.bounding_sphere = std::max(glm::length(pos), rv.bounding_sphere);
    }

    glBindBuffer(GL_ARRAY_BUFFER, rv.vertex_buffer.get());
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(rv.vao.get());
    SUSHI_DEFER { glBindVertexArray(0); };

    auto stride = sizeof(GLfloat) * (3 + 2 + 3);
    glEnableVertexAttribArray(attrib_location::POSITION);
    glEnableVertexAttribArray(attrib_location::TEXCOORD);
    glEnableVertexAttribArray(attrib_location::NORMAL);
    glVertexAttribPointer(
        attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(0));
    glVertexAttribPointer(
        attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(
        attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * (3 + 2)));

    return rv;
}

} // static

static_mesh load_static_mesh_file(const std::string &fname) {
    std::ifstream file(fname);
    std::string line;
    std::string word;
    int line_number = 0;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<GLfloat> data;
    int num_tris = 0;

    while (getline(file, line)) {
        ++line_number;
        std::istringstream iss(line);
        iss >> word;

        if (word == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "vn") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            normals.push_back(v);
        } else if (word == "vt") {
            glm::vec2 v;
            iss >> v.x >> v.y;
            v.y = 1.0 - v.y;
            texcoords.push_back(v);
        } else if (word == "f") {
            int index;
            for (int i = 0; i < 3; ++i) {
                iss >> word;
                std::replace(begin(word), end(word), '/', ' ');
                std::istringstream iss2(word);
                iss2 >> index;
                --index;
                data.push_back(vertices[index].x);
                data.push_back(vertices[index].y);
                data.push_back(vertices[index].z);
                iss2 >> index;
                --index;
                data.push_back(texcoords[index].x);
                data.push_back(texcoords[index].y);
                iss2 >> index;
                --index;
                data.push_back(normals[index].x);
                data.push_back(normals[index].y);
                data.push_back(normals[index].z);
            }
            ++num_tris;
        } else if (word[0] == '#') {
            // pass
        } else {
            std::clog << "sushi::load_static_mesh_file(): Warning: Unknown OBJ directive at " << fname << "[" <<
            line_number
            << "]: \"" << word << "\"." << std::endl;
        }
    }

    return upload_static_mesh(data, num_tris);
}

static_mesh load_static_mesh_data(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords, const std::vector<Tri>& tris) {
    std::vector<GLfloat> data;
    int num_tris = tris.size();

    for (const auto& tri : tris) {
        for (const auto& vert : tri.verts) {
            data.push_back(positions[vert.pos].x);
            data.push_back(positions[vert.pos].y);
            data.push_back(positions[vert.pos].z);
            data.push_back(texcoords[vert.tex].x);
            data.push_back(texcoords[vert.tex].y);
            data.push_back(normals[vert.norm].x);
            data.push_back(normals[vert.norm].y);
            data.push_back(normals[vert.norm].z);
        }
    }

    return upload_static_mesh(data, num_tris);
}

animated_mesh::animated_mesh(std::shared_ptr<const Source> source, const Mesh* mesh) : source(std::move(source)), mesh(mesh) {}

void animated_mesh::set_anim(const std::string& name) {
    anim = &source->anims.at(name);
    time = 0.f;
}

std::string animated_mesh::get_anim() const {
    if (anim) {
        return anim->name;
    } else {
        return "";
    }
}

void animated_mesh::update(float delta) {
    if (!anim) {
        return;
    }

    auto frame = int(time * anim->framerate);

    if (anim->loop) {
        frame = frame % anim->num_frames;
    } else {
        frame = std::min(frame, anim->num_frames - 1);
    }

    time += delta;

    out_frames.clear();
    for (auto i = 0u; i < source->bone_mats.size(); ++i) {
        auto mat = source->frame_mats[source->bone_mats.size() * (anim->first_frame + frame) + i];
        if(source->bone_parents[i] >= 0) {
            mat = out_frames[source->bone_parents[i]] * mat;
        }
        out_frames.push_back(mat);
    }
}

animated_mesh_factory::animated_mesh_factory(std::shared_ptr<animated_mesh::Source> source) : source(std::move(source)) {}

animated_mesh_factory::animated_mesh_factory(const iqm::iqm_data& data) {
    auto pos_arr = data.vertexarrays.position;
    auto norm_arr = data.vertexarrays.normal;
    auto tex_arr = data.vertexarrays.texcoord;
    auto idex_arr = data.vertexarrays.blendindexes;
    auto weight_arr = data.vertexarrays.blendweights;

    // anims

    for (auto i = 0u; i < data.anims.size(); ++i) {
        source->anims.insert(std::make_pair(
            data.anims[i].name,
            animated_mesh::Anim{
                data.anims[i].name,
                (int)data.anims[i].first_frame,
                (int)data.anims[i].num_frames,
                data.anims[i].framerate,
                data.anims[i].loop
            }
        ));
    }

    // bones

    for (auto i = 0u; i < data.joints.size(); ++i) {
        auto& j = data.joints[i];
        source->bone_parents.push_back(j.parent);
        auto mat = glm::mat4(1.f);
        mat = glm::translate(mat, j.pos);
        mat = mat * glm::mat4_cast(glm::normalize(j.rot));
        mat = glm::scale(mat, j.scl);

        if(j.parent >= 0)
        {
            mat = source->bone_mats[j.parent] * mat;
        }
        source->bone_mats.push_back(mat);
    }

    // frames

    auto frame_i = 0u;

    for (auto i = 0u; i < data.frames.size() / data.num_framechannels; ++i) {
        for (auto j = 0u; j < data.poses.size(); ++j) {
            auto pose_pos = glm::vec3();
            auto pose_rot = glm::quat();
            auto pose_scl = glm::vec3();
            float* chans[10] = {
                &pose_pos.x, &pose_pos.y, &pose_pos.z,
                &pose_rot.x, &pose_rot.y, &pose_rot.z, &pose_rot.w,
                &pose_scl.x, &pose_scl.y, &pose_scl.z,
            };
            auto p = &data.poses[j];
            for (int i=0; i<10; ++i) {
                *chans[i] = p->offsets[i];
                if (p->channels[i]) {
                    *chans[i] += data.frames[frame_i++] * p->scales[i];
                }
            }

            auto mat = glm::mat4();
            mat = glm::translate(mat, pose_pos);
            mat = mat * glm::mat4_cast(glm::normalize(pose_rot));
            mat = glm::scale(mat, pose_scl);

            if (p->parent >= 0) {
                mat = source->bone_mats[p->parent] * mat;
            }
            mat = mat * glm::inverse(source->bone_mats[j]);

            source->frame_mats.push_back(mat);
        }
    }

    // meshes

    for (auto& m : data.meshes) {
        auto& mesh = source->meshes[m.name];

        mesh.num_tris = m.num_triangles;

        glBindVertexArray(mesh.vao.get());
        SUSHI_DEFER { glBindVertexArray(0); };
        SUSHI_DEFER { glBindBuffer(GL_ARRAY_BUFFER, 0); };

        glEnableVertexAttribArray(sushi::attrib_location::POSITION);
        glEnableVertexAttribArray(sushi::attrib_location::TEXCOORD);
        glEnableVertexAttribArray(sushi::attrib_location::NORMAL);
        glEnableVertexAttribArray(3); // indices
        glEnableVertexAttribArray(4); // weights

        glBindBuffer(GL_ARRAY_BUFFER, mesh.pos_vb.get());
        glBufferData(GL_ARRAY_BUFFER, m.num_vertexes * 3 * sizeof(GLfloat), &pos_arr[m.first_vertex*3], GL_STATIC_DRAW);
        glVertexAttribPointer(
            sushi::attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid *>(0));

        glBindBuffer(GL_ARRAY_BUFFER, mesh.tex_vb.get());
        glBufferData(GL_ARRAY_BUFFER, m.num_vertexes * 2 * sizeof(GLfloat), &tex_arr[m.first_vertex*2], GL_STATIC_DRAW);
        glVertexAttribPointer(
            sushi::attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid *>(0));

        glBindBuffer(GL_ARRAY_BUFFER, mesh.norm_vb.get());
        glBufferData(GL_ARRAY_BUFFER, m.num_vertexes * 3 * sizeof(GLfloat), &norm_arr[m.first_vertex*3], GL_STATIC_DRAW);
        glVertexAttribPointer(
            sushi::attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, 0,
            reinterpret_cast<const GLvoid *>(0));

        glBindBuffer(GL_ARRAY_BUFFER, mesh.idex_vb.get());
        glBufferData(GL_ARRAY_BUFFER, m.num_vertexes * 4, &idex_arr[m.first_vertex*4], GL_STATIC_DRAW);
        glVertexAttribPointer(
            3, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0,
            reinterpret_cast<const GLvoid *>(0));

        glBindBuffer(GL_ARRAY_BUFFER, mesh.weight_vb.get());
        glBufferData(GL_ARRAY_BUFFER, m.num_vertexes * 4, &weight_arr[m.first_vertex*4], GL_STATIC_DRAW);
        glVertexAttribPointer(
            4, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,
            reinterpret_cast<const GLvoid *>(0));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tris.get());
        SUSHI_DEFER { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.num_triangles * 3 * 4, &data.triangles[m.first_triangle], GL_STATIC_DRAW);
    }
}

animated_mesh animated_mesh_factory::get(const std::string& name) {
    return animated_mesh(source, &source->meshes.at(name));
}

} // namespace sushi
