#version 400

in vec2 v_texcoord;
in vec3 v_normal;

uniform sampler2D s_texture;
uniform vec3 cam_forward;
uniform vec4 tint;

layout(location = 0) out vec4 color;

void main() {
    if (dot(v_normal, cam_forward) > 0.0) discard;
    color = texture(s_texture, v_texcoord) * tint;
}
