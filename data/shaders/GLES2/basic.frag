precision mediump float;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform sampler2D s_texture;
uniform vec3 cam_forward;
uniform vec4 tint;

void main()
{
    if (dot(v_normal, cam_forward) > 0.0) discard;
    vec4 color = texture2D(s_texture, v_texcoord) * tint;
    if (color.a < 0.5) discard;
    gl_FragColor = color;
}
