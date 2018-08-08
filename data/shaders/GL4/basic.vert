#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

out vec2 v_texcoord;
out vec3 v_normal;

uniform mat4 MVP;
uniform mat4 normal_mat;

void main() {
    v_texcoord = texcoord;
    v_normal = vec3(normal_mat * vec4(normal, 0.0));
    gl_Position = MVP * vec4(position, 1.0);
}
