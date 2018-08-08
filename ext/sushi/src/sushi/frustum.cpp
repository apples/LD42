
#include "frustum.hpp"

namespace sushi {

frustum::plane::plane(const glm::vec4 &vec) :
    normal(vec),
    offset(vec.w)
{
    auto len = length(normal);
    normal /= len;
    offset /= len;
}

frustum::frustum(const glm::mat4& view_proj_mat) :
    planes{
        plane(row(view_proj_mat, 3) + row(view_proj_mat, 0)),
        plane(row(view_proj_mat, 3) - row(view_proj_mat, 0)),
        plane(row(view_proj_mat, 3) + row(view_proj_mat, 1)),
        plane(row(view_proj_mat, 3) - row(view_proj_mat, 1)),
        plane(row(view_proj_mat, 3) + row(view_proj_mat, 2)),
        plane(row(view_proj_mat, 3) - row(view_proj_mat, 2)),
    }
{}

bool frustum::contains(const glm::vec3& position, float radius) const {
    for (auto&& plane : planes) {
        if (dot(plane.normal, position) + plane.offset + radius < 0) {
            return false;
        }
    }
    return true;
}

} // namespace sushi
