#version 330 core

#pragma include("camera.glsl")
#pragma include("raymarching.glsl")

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

// Scene distance function
// Returns float distance
float map(vec3 p) {
    float d = length(p - vec3(0.0, 0.5, 0.0)) - 0.5;
    d = min(d, dot(p, vec3(0.0, 1.0, 0.0)) + 1.0);
    return d;
}

void main() {
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;

    vec3 ro = vec3(0.0, 0.5, 2.0);
    vec3 target = vec3(0.0, 0.5, 0.0);
    vec3 rd;

    editorCamera(ro, target, uv, rd);
    demoCamera(ro, target, uv, rd);

    vec2 hit = intersect(ro, rd);
    float t = hit.x;
    float dist = hit.y;

    vec3 col = vec3(0.0);

    if (t < RAYMARCH_MAX_DISTANCE && dist < RAYMARCH_MIN_DISTANCE * 2.0) {
        vec3 p = ro + rd * t;
        vec3 n = normal(p);
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        
        float diff = max(dot(n, lightDir), 0.0);
        col = vec3(0.5, 0.6, 0.7) * (diff + 0.1);
        col *= softshadow(p, lightDir, 0.02, 2.5, 8.0);
    } else {
        col = vec3(0.1, 0.1, 0.15) - rd.y * 0.1;
    }

    FragColor = vec4(col, t);
}
