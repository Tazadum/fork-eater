#version 330 core

#pragma group("Colors")
#pragma slider(BRIGHTNESS, 0.1, 1.0, 0.5, "Brightness")
#pragma endgroup()

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Classic rainbow palette
    vec3 col = BRIGHTNESS + BRIGHTNESS * cos(iTime + uv.xyx + vec3(0, 2, 4));
    
    FragColor = vec4(col, 1.0);
}
