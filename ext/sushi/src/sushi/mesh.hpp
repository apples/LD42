//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_MESH_HPP
#define SUSHI_MESH_HPP

#include "gl.hpp"
#include "common.hpp"
#include "iqm.hpp"

#include <string>
#include <memory>
#include <map>
#include <vector>

/// Sushi
namespace sushi {

/// Deleter for OpenGL buffer objects.
struct buffer_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteBuffers(1, &buf);
    }
};

/// Deleter for OpenGL vertex array objects.
struct vertex_array_deleter {
    using pointer = fake_nullable<GLuint>;

    void operator()(pointer p) const {
        auto buf = GLuint(p);
        glDeleteVertexArrays(1, &buf);
    }
};

/// A unique handle to an OpenGL buffer object.
using unique_buffer = unique_gl_resource<buffer_deleter>;

/// A unique handle to an OpenGL vertex array object.
using unique_vertex_array = unique_gl_resource<vertex_array_deleter>;

/// Creates a unique OpenGL buffer object.
/// \return A unique buffer object.
inline unique_buffer make_unique_buffer() {
    GLuint buf;
    glGenBuffers(1, &buf);
    return unique_buffer(buf);
}

/// Creates a unique OpenGL vertex array object.
/// \return A unique vertex array object.
inline unique_vertex_array make_unique_vertex_array() {
    GLuint buf;
    glGenVertexArrays(1, &buf);
    return unique_vertex_array(buf);
}

/// A static OpenGL mesh made of triangles.
struct static_mesh {
    unique_vertex_array vao;
    unique_buffer vertex_buffer;
    int num_triangles = 0;
    float bounding_sphere = 0;
};

struct attrib_location {
    static constexpr auto POSITION = 0;
    static constexpr auto TEXCOORD = 1;
    static constexpr auto NORMAL = 2;
};

/// Loads a mesh from an OBJ file.
/// The following OBJ directives are supported:
/// - `#` - Comments
/// - `v` - Vertex location.
/// - `vn` - Vertex normal.
/// - `vt` - Vertex texture coordinate.
/// - `f` - Face (triangles only).
/// \param fname File name.
/// \return The static mesh described by the file.
static_mesh load_static_mesh_file(const std::string &fname);

struct Tri {
    using Index3 = std::vector<glm::vec3>::size_type;
    using Index2 = std::vector<glm::vec2>::size_type;
    struct Vert {
        Index3 pos;
        Index3 norm;
        Index2 tex;
    };
    Vert verts[3];
};

static_mesh load_static_mesh_data(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords, const std::vector<Tri>& tris);

/// An animated mesh designed for GPU skinning techniques.
struct animated_mesh {
    struct Mesh {
        unique_buffer pos_vb = make_unique_buffer();
        unique_buffer norm_vb = make_unique_buffer();
        unique_buffer tex_vb = make_unique_buffer();
        unique_buffer idex_vb = make_unique_buffer();
        unique_buffer weight_vb = make_unique_buffer();
        unique_vertex_array vao = make_unique_vertex_array();
        unique_buffer tris = make_unique_buffer();
        int num_tris = 0;
    };
    struct Anim {
        std::string name;
        int first_frame;
        int num_frames;
        float framerate;
        bool loop;
    };
    struct Source {
        std::vector<int> bone_parents;
        std::vector<glm::mat4> bone_mats;
        std::vector<glm::mat4> frame_mats;
        std::map<std::string,Anim> anims;
        std::map<std::string,Mesh> meshes;
    };

    std::shared_ptr<const Source> source;
    const Mesh* mesh;
    const Anim* anim = nullptr;
    std::vector<glm::mat4> out_frames;
    float time = 0.f;

    /// Used by animated_mesh_factory.
    animated_mesh(std::shared_ptr<const Source> source, const Mesh* mesh);

    /// Resets the animation.
    /// Resets the frame counter to zero and changes the current animation.
    /// If
    void set_anim(const std::string& name);

    /// Gets the name of the current animation.
    std::string get_anim() const;

    /// Updates the animation frame.
    /// \param delta The time since the last update.
    void update(float delta);
};

/// An animated_mesh factory designed for use with the IQM format.
struct animated_mesh_factory {
    std::shared_ptr<animated_mesh::Source> source = std::make_shared<animated_mesh::Source>();

    /// Creates a factory from an existing Source.
    animated_mesh_factory(std::shared_ptr<animated_mesh::Source> source);

    /// Creates a factory for meshes loaded from an IQM file.
    animated_mesh_factory(const iqm::iqm_data& data);

    /// Gets a mesh by name.
    animated_mesh get(const std::string& name);
};

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const static_mesh& mesh) {
    glBindVertexArray(mesh.vao.get());
    SUSHI_DEFER { glBindVertexArray(0); };
    glDrawArrays(GL_TRIANGLES, 0, mesh.num_triangles * 3);
}

/// Draws a mesh.
/// \param mesh The mesh to draw.
inline void draw_mesh(const animated_mesh& mesh) {
    if (mesh.out_frames.size() > 32) throw;
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindVertexArray(mesh.mesh->vao.get());
    SUSHI_DEFER { glBindVertexArray(0); };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.mesh->tris.get());
    SUSHI_DEFER { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); };
    glUniformMatrix4fv(glGetUniformLocation(program, "Bones"), mesh.out_frames.size(), GL_FALSE, (GLfloat*)&mesh.out_frames[0]);
    glDrawElements(GL_TRIANGLES, mesh.mesh->num_tris*3, GL_UNSIGNED_INT, nullptr);
}

} // namespace sushi

#endif //SUSHI_MESH_HPP
