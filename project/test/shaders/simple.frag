#version 330 core
out vec4 FragColor;

uniform vec3 iResolution;

void main()
{
    FragColor = vec4(iResolution.xy / vec2(1920.0, 1080.0), 0.0, 1.0);
}
