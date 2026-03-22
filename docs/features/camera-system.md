# Camera System

The Fork Eater camera system provides a robust set of tools for navigating 3D scenes. It includes support for both **Orbital** and **Free Look** (Flying) modes, integrated with WASD keyboard controls and automatic persistence.

## Core Features

- **Standard 3D Navigation**: WASD controls for flying in the camera direction.
- **Orbital Mode**: Lock the camera around a specific target with adjustable radius.
- **DPI-Aware Mouse Support**: Relative mouse movement (`u_fork_cam_mouse`) is used to rotate the view.
- **Field of View (FOV)**: Adjustable FOV in degrees (10° to 150°) with automatic focal length calculation.
- **Movement Modifiers**: 
    - **Shift**: Fast movement (3x speed).
    - **Ctrl**: Slow/Precision movement (0.1x speed).
- **Persistence**: Camera position is automatically saved to `.4k-eater.local` in the project directory.

## Integration in Shaders

To use the camera system in your fragment shader, include the `camera.glsl` library:

```glsl
#pragma include("camera.glsl")

uniform vec3 u_fork_cam_pos;
uniform vec3 u_fork_cam_target;
uniform vec2 u_fork_cam_mouse;

void main() {
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;
    
    vec3 ro = u_fork_cam_pos; // Managed by C++ and moved with WASD
    vec3 target = u_fork_cam_target; // Managed by C++ and moved with WASD
    vec3 rd;
    
    // Automatic editor/demo camera switching
    editorCamera(ro, target, uv, rd);
    demoCamera(ro, target, uv, rd);
    
    // ... use ro and rd for raymarching ...
}
```

## Control Modes

### Free Look (Flying)
When `USE_ORBITAL_CAMERA` is disabled (Free Look mode):
- **W / S**: Move forward and backward in the direction the camera is looking.
- **A / D**: Strafe left and right relative to the camera orientation.
- **Mouse**: Rotate the view.

### Orbital Mode
When `USE_ORBITAL_CAMERA` is enabled:
- **W / S**: Increase or decrease the orbital radius (move closer or further from the target).
- **Mouse**: Rotate around the target.

## Technical Details

- **Focal Length**: Calculated as `1.0 / tan(fov_angle / 2)`.
- **Position Uniform**: `u_fork_cam_pos` (vec3) provides the world-space camera origin.
- **Target Uniform**: `u_fork_cam_target` (vec3) provides the world-space camera target (focus point).
- **Mouse Uniform**: `u_fork_cam_mouse` (vec2) provides the integrated relative mouse movement.
- **Local Persistence**: The `.4k-eater.local` file stores the last known camera position and target for each project:
  ```properties
  cam_pos=1.24 0.50 3.82
  cam_target=0.00 0.50 0.00
  ```

## Library Functions (camera.glsl)

- `editorCamera(inout vec3 ro, inout vec3 ta, vec2 uv, inout vec3 rd)`: The primary entry point for camera setup.
- `getFocalLength()`: Returns the perspective zoom factor based on the current FOV.
- `orbitalCamera4k(...)` & `freeLookCamera4k(...)`: Internal implementations of the camera modes.
