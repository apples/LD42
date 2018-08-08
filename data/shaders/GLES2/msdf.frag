#extension GL_OES_standard_derivatives : enable
precision mediump float;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform sampler2D msdf;
uniform float pxRange;
uniform vec2 texSize;
uniform vec4 fgColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec2 msdfUnit = pxRange/texSize;
    vec3 sample = texture2D(msdf, v_texcoord).rgb;
    float sigDist = median(sample.r, sample.g, sample.b) - 0.5;
    sigDist *= dot(msdfUnit, 0.5/fwidth(v_texcoord));
    float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
    gl_FragColor = vec4(fgColor.rgb, fgColor.a*opacity);
}
