#version 330 core

#pragma include(libs/camera.glsl)
#pragma include(libs/raymarching_vec3.glsl)

out vec4 FragColor;

in vec2 TexCoord;

uniform float iTime;
uniform vec3 iResolution;

// Distance function for a sphere
float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

// Distance function for a plane
float sdPlane(vec3 p, vec4 n) {
    return dot(p, n.xyz) + n.w;
}

// Scene distance function
// Returns vec4(distance, dummy, material_id, emissive)
vec4 map(vec3 p) {
    float d1 = sdPlane(p, vec4(0.0, 1.0, 0.0, 1.0));
    float d2 = sdSphere(p - vec3(0.0, 0.5, 0.0), 0.5);
    
    if (d1 < d2) {
        return vec4(d1, 0.0, 1.0, 0.0); // Floor material 1
    } else {
        return vec4(d2, 0.0, 2.0, 0.0); // Sphere material 2
    }
}

void main() {
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;

    // Default camera position and target
    vec3 ro = vec3(0.0, 0.5, 2.0);
    vec3 target = vec3(0.0, 0.5, 0.0);
    vec3 rd;

    // Use library camera (supports both editor orbital and demo fixed camera)
    editorCamera(ro, target, uv, rd);
    demoCamera(ro, target, uv, rd);

    // Use library raymarching
    vec4 hit = intersect(ro, rd);
    float t = hit.x;
    float matId = hit.z;

    vec3 col = vec3(0.0);
    float depth = t;

    if (matId > -0.5) {
        vec3 p = ro + rd * t;
        vec3 n = normal(p);
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        
        // Basic lighting
        float diff = max(dot(n, lightDir), 0.0);
        float amb = 0.1;
        
        // Color based on material ID
        vec3 baseCol = (matId < 1.5) ? vec3(0.2, 0.3, 0.4) : vec3(0.8, 0.4, 0.1);
        col = baseCol * (diff + amb);
        
        // Add soft shadows if enabled in library
        col *= softshadow(p, lightDir, 0.02, 2.5, 8.0);
    } else {
        // Background / Sky
        col = vec3(0.1, 0.1, 0.15) - rd.y * 0.1;
        depth = RAYMARCH_MAX_DISTANCE;
    }

    FragColor = vec4(col, depth);
}
