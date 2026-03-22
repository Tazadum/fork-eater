/*

*/

#pragma group("Camera")
#pragma switch(USE_CAMERA, true, "Disable", "Enable")
#pragma switch(USE_ORBITAL_CAMERA, true, "Free look", "Orbital")
#pragma slider(FORK_CAMERA_FOV, 10.0, 150.0, 60.0, "FOV")
#pragma endgroup()

#ifndef FORK_CAMERA_FOV
#define FORK_CAMERA_FOV 60.0
#endif

#ifdef USE_CAMERA
uniform vec2 u_fork_cam_mouse;
uniform vec3 u_fork_cam_pos;
#define FORK_CAMERA_MOUSE_SENSITIVITY 3.14

mat2 fork_rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Calculate focal length from FOV (degrees)
float getFocalLength() {
    return 1.0 / tan(FORK_CAMERA_FOV * 0.5 * 3.14159265 / 180.0);
}

void freeLookCamera4k(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec2 m = u_fork_cam_mouse;
    
    // Initial orientation based on target
    vec3 fwd = normalize(ta - ro);           
    
    // Rotate fwd based on mouse input
    float theta = -m.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    float phi = clamp(-m.y * FORK_CAMERA_MOUSE_SENSITIVITY, -1.5, 1.5);
    
    // Calculate angles of initial fwd
    float base_theta = atan(fwd.x, fwd.z);
    float base_phi = asin(clamp(fwd.y, -1.0, 1.0));
    
    float final_theta = base_theta + theta;
    float final_phi = clamp(base_phi + phi, -1.57, 1.57);
    
    vec3 final_fwd = vec3(
        cos(final_phi) * sin(final_theta),
        sin(final_phi),
        cos(final_phi) * cos(final_theta)
    );
    
    vec3 final_right = normalize(cross(vec3(0.0, 1.0, 0.0), final_fwd));
    vec3 final_up = cross(final_fwd, final_right);
    
    rd = normalize(uv.x * final_right + uv.y * final_up + getFocalLength() * final_fwd);
}

void orbitalCamera4k(inout vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec2 m = u_fork_cam_mouse;
    vec3 p = ro - ta;
    float r = length(p);
    
    // Get current angles
    float phi = acos(clamp(p.y / r, -1.0, 1.0));
    float theta = atan(p.x, p.z);
    
    // Offset by mouse
    phi = clamp(phi - m.y * FORK_CAMERA_MOUSE_SENSITIVITY, 0.01, 3.1415);
    theta = theta - m.x * FORK_CAMERA_MOUSE_SENSITIVITY;
    
    // New camera position
    ro = ta + vec3(r * sin(phi) * sin(theta), r * cos(phi), r * sin(phi) * cos(theta));
    
    // Orientation
    vec3 fwd = normalize(ta - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));
    vec3 up = cross(fwd, right);
    
    rd = normalize(uv.x * right + uv.y * up + getFocalLength() * fwd);
}

#endif

#ifdef USE_CAMERA

void editorCamera(inout vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    ro = u_fork_cam_pos;
    #ifdef USE_ORBITAL_CAMERA
        orbitalCamera4k(ro, ta, uv, rd);
    #else
        freeLookCamera4k(ro, ta, uv, rd);
    #endif
}

void demoCamera(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec3 f = normalize(ta - ro);           
    vec3 r = normalize(cross(abs(f.y) > 0.99 ? vec3(0,0,1) : vec3(0,1,0), f));
    vec3 u = cross(f, r);
    rd = normalize(uv.x * r + uv.y * u + getFocalLength() * f);
}

#else

void editorCamera(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    demoCamera(ro, ta, uv, rd);
}

void demoCamera(vec3 ro, vec3 ta, vec2 uv, out vec3 rd) {
    vec3 f = normalize(ta - ro);           
    vec3 r = normalize(cross(abs(f.y) > 0.99 ? vec3(0,0,1) : vec3(0,1,0), f));
    vec3 u = cross(f, r);
    rd = normalize(uv.x * r + uv.y * u + getFocalLength() * f);
}
#endif
