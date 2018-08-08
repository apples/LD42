#version 410

in vec2 TexCoord;

uniform sampler2D DiffuseTexture;
uniform bool GrayScale;

out vec4 FragColor;

void main() {
    FragColor = texture(DiffuseTexture, TexCoord);
    if (GrayScale) {
        float gray = dot(FragColor.rgb, vec3(0.299, 0.587, 0.114));
        FragColor = vec4(vec3(gray), gl_Color.a);
    }
}
