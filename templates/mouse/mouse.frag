#version 330 core

#pragma group("Mouse Settings")
#pragma slider(CIRCLE_SIZE, 0.01, 0.2, 0.05, "Circle Size")
#pragma slider(SMOOTHNESS, 0.0, 0.1, 0.01, "Edge Softness")
#pragma endgroup()

#pragma include(libs/utils.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 iResolution;
uniform float iTime;
uniform vec4 iMouse;

void main()
{
    vec2 uv = TexCoord;
    vec2 mouse = iMouse.xy;

    // Background gradient based on mouse
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4) + mouse.xyx);

    // Dynamic circle at mouse position
    float d = distance(uv, mouse);
    float circleMask = 1.0 - smoothstep(CIRCLE_SIZE - SMOOTHNESS, CIRCLE_SIZE + SMOOTHNESS, d);
    
    // Mix background with circle color
    vec3 circleCol = vec3(1.0, 0.8, 0.0) * (sin(iTime * 10.0) * 0.2 + 0.8);
    col = mix(col, circleCol, circleMask);

    FragColor = vec4(col, 1.0);
}
