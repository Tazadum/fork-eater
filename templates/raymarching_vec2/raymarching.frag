#version 330 core

#pragma include(libs/camera.glsl)
#pragma include(libs/raymarching_vec2.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

// Scene distance function
// Returns vec2(distance, material_id)
vec2 map(vec3 p) {
    float d1 = length(p - vec3(0.0, 0.5, 0.0)) - 0.5;
    float d2 = dot(p, vec3(0.0, 1.0, 0.0)) + 1.0;
    
    if (d1 < d2) {
        return vec2(d1, 1.0); // Sphere
    } else {
        return vec2(d2, 2.0); // Floor
    }
}

void main() {
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;

    vec3 ro = vec3(0.0, 0.5, 2.0);
    vec3 target = vec3(0.0, 0.5, 0.0);
    vec3 rd;

    editorCamera(ro, target, uv, rd);
    demoCamera(ro, target, uv, rd);

    vec3 hit = intersect(ro, rd);
    float t = hit.x;
    float matId = hit.z;

    vec3 col = vec3(0.0);

    if (matId > 0.0) {
        vec3 p = ro + rd * t;
        vec3 n = normal(p);
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        
        float diff = max(dot(n, lightDir), 0.0);
        vec3 baseCol = (matId < 1.5) ? vec3(0.8, 0.2, 0.1) : vec3(0.2, 0.2, 0.2);
        
        col = baseCol * (diff + 0.1);
        col *= softshadow(p, lightDir, 0.02, 2.5, 8.0);
    } else {
        col = vec3(0.1, 0.1, 0.15) - rd.y * 0.1;
    }

    FragColor = vec4(col, t);
}
