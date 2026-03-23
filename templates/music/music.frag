#version 330 core

#pragma group("Music Configuration")
#pragma slider(BPM, 60.0, 200.0, 128.0, "BPM")
#pragma slider(DIVIDE, 1.0, 16.0, 4.0, "Beat Division")
#pragma slider(SPARKLE_INTENSITY, 0.0, 1.0, 0.5, "Sparkle Intensity")
#pragma endgroup()

#pragma include(libs/color.glsl)
#pragma include(libs/noise.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

void main()
{
    vec2 uv = TexCoord;
    
    // Convert to beats
    float beat = iTime * BPM / 60.0;
    float beatPulse = sin(beat * 3.14159) * 0.5 + 0.5;
    float subBeat = sin(beat * 3.14159 * (DIVIDE / 4.0)) * 0.5 + 0.5;
    
    // Animated radial pattern
    vec2 center = vec2(0.5, 0.5);
    float dist = length(uv - center);
    float circles = sin(dist * 20.0 - beat * 4.0) * beatPulse;
    
    // Beat-synchronized HSV to RGB conversion
    vec3 hsv = vec3(fract(beat * 0.1), 0.7, 0.8);
    vec3 baseColor = hsv2rgb(hsv);
    
    // Secondary pulsing color
    vec3 color2 = hsv2rgb(vec3(fract(beat * 0.1 + 0.5), 0.6, 0.7));
    
    vec3 finalColor = mix(baseColor, color2, sin(uv.x * 6.28 + beat)) * (circles + 0.2);
    finalColor = mix(finalColor, vec3(1.0), subBeat * 0.1);
    
    // Use library noise for sparkles
    float sparkle = noise31(vec3(uv * 100.0, iTime * 2.0));
    if (sparkle > (1.0 - SPARKLE_INTENSITY * 0.05)) {
        finalColor += vec3(1.0) * beatPulse;
    }
    
    FragColor = vec4(gamma(finalColor), 1.0);
}
