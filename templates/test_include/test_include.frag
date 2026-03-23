#version 330 core

#pragma include(libs/utils.glsl)

out vec4 FragColor;

in vec2 TexCoord;

void main() {
    float c = circle(TexCoord, 0.3);
    FragColor = vec4(c, c, c, 1.0);
}
