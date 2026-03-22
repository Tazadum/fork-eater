# Raymarching Libraries

Fork Eater provides a set of optimized raymarching libraries to accelerate shader development. These libraries handle common tasks like intersection testing, normal calculation, and soft shadows, supporting various return types for the distance field (`map`) function.

## Core Libraries

| Library File | `map(vec3)` Return Type | `intersect(ro, rd)` Return Type |
| :--- | :--- | :--- |
| `raymarching.glsl` | `float` (distance) | `vec2(t, distance)` |
| `raymarching_vec2.glsl` | `vec2(distance, id)` | `vec3(t, distance, id)` |
| `raymarching_vec3.glsl` | `vec4(distance, dummy, id, emissive)` | `vec4(t, distance, id, emissive)` |

## Key Features

- **Optimized Intersectors**: Includes standard and "relaxed" tracing modes.
- **Normal Calculation**: Automatic normal estimation using the finite difference method (`normal(vec3 p)`).
- **Soft Shadows**: Built-in support for high-quality soft shadows with adjustable penumbra.
- **Configurable**: Adjustable via `#pragma` sliders for steps, max distance, and shadow quality.
- **Internal Marching**: Support for marching inside surfaces (useful for refractive effects).

## Configuration Pragmas

The libraries include `#pragma` definitions that automatically create UI sliders in the editor:
- `RAYMARCH_STEPS`: Maximum number of steps per ray (default 100).
- `RAYMARCH_MAX_DISTANCE`: Maximum distance a ray can travel.
- `RAYMARCH_RELAXED`: Switch between standard and relaxed (faster convergence) tracing.
- `RAYMARCH_SOFT_SHADE_STEPS`: Number of steps for soft shadow calculation.

## Basic Usage Example

```glsl
#pragma include("raymarching.glsl")

float map(vec3 p) {
    return length(p) - 1.0; // Sphere at origin
}

void main() {
    // ... camera setup ...
    vec2 hit = intersect(ro, rd);
    if (hit.y < 0.001) {
        vec3 p = ro + rd * hit.x;
        vec3 n = normal(p);
        // ... shading ...
    }
}
```

## Material IDs and Data

For more complex scenes, use `raymarching_vec2.glsl` or `raymarching_vec3.glsl`:

```glsl
#pragma include("raymarching_vec2.glsl")

vec2 map(vec3 p) {
    float d1 = length(p) - 1.0;
    float d2 = p.y + 1.0;
    if (d1 < d2) return vec2(d1, 1.0); // ID 1.0
    return vec2(d2, 2.0); // ID 2.0
}
```
