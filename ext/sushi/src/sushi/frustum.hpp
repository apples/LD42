#ifndef SUSHI_FRUSTUM_HPP
#define SUSHI_FRUSTUM_HPP

#include "gl.hpp"

namespace sushi {

/// A camera frustum.
class frustum {
public:
    /// A frustum plane.
    struct plane {
        glm::vec3 normal = {0, 0, 0};
        float offset = 0;

        plane() = default;
        plane(const glm::vec4& vec);
    };

    /// The six planes, in the same order as the planes array.
    enum plane_type {
        LEFT,
        RIGHT,
        BOTTOM,
        TOP,
        FRONT,
        BACK
    };

    /// The six planes of the frustum, in the same order as the values of plane_type.
    plane planes[6];

    /// Default constructor.
    frustum() = default;

    /// Constructs a frustum representing the bounds of the given view-projection matrix.
    /// \param view_proj_mat View-projection matrix.
    frustum(const glm::mat4& view_proj_mat);

    /// Determines if the sphere located at the given position with the given radius intersects the frustum.
    /// \param position Position of sphere.
    /// \param radius Radius of sphere.
    /// \return True if the sphere intersects the frustum.
    bool contains(const glm::vec3& position, float radius) const;
};

} // namespace sushi

#endif // SUSHI_FRUSTUM_HPP
