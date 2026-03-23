#version 330 core

#pragma group("Shapes")
#pragma slider(RADIUS, 0.1, 0.5, 0.25, "Circle Radius")
#pragma endgroup()

#pragma include(libs/utils.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

void main() {
    vec2 uv = TexCoord;
    
    // Use circle function from utils.glsl
    float c = circle(uv, RADIUS);
    
    vec3 col = vec3(c);
    col *= 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    
    FragColor = vec4(col, 1.0);
}
