#version 330 core

#pragma group("Animation")
#pragma slider(SPEED, 0.1, 5.0, 1.0, "Animation Speed")
#pragma slider(COLOR_FREQ, 0.1, 10.0, 1.0, "Color Frequency")
#pragma endgroup()

#pragma include(libs/color.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Base animated color using hsv2rgb from library
    float t = iTime * SPEED;
    vec3 hsv = vec3(fract(t * 0.1 + uv.x * 0.2), 0.7, 0.8);
    vec3 col = hsv2rgb(hsv);
    
    // Add wave pattern
    float wave = sin(uv.x * COLOR_FREQ * 10.0 + t) * sin(uv.y * COLOR_FREQ * 10.0 + t * 0.5);
    col += 0.2 * wave;
    
    // Apply gamma correction from library
    FragColor = vec4(gamma(col), 1.0);
}
