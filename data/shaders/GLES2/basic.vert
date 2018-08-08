attribute vec3 position;
attribute vec2 texcoord;
attribute vec3 normal;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform mat4 MVP;
uniform mat4 normal_mat;

void main()
{
    v_texcoord = texcoord;
    v_normal = vec3(normal_mat * vec4(normal, 0.0));
    gl_Position = MVP * vec4(position, 1.0);
}
