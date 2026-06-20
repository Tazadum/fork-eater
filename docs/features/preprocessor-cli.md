# Command-Line Preprocessor Mode

The Fork Eater command-line preprocessor mode allows you to resolve shader includes, expand interactive widgets (sliders and switches) into standard macros, replace resolution variables with static macro constants, and bake uniform variables with values loaded from `uniforms.json`.

This mode is designed for embedding shaders into 4k intros, exporting final compilable GLSL sources for standalone engines, or offline shader verification.

---

## Command Line Arguments

To run the preprocessor mode, specify the output GLSL file path with `-o` or `--output`. The preprocessor will run and exit without initializing the GLFW window, ImGui UI, or OpenGL contexts.

| Switch | Argument | Description |
| :--- | :--- | :--- |
| `-p`, `--preprocess` | `path` | Path to a shader file (.frag) or a project directory. |
| `-o`, `--output` | `path` | Destination path for the preprocessed and baked GLSL source. |
| `-w`, `--width` | `integer` | Horizontal resolution value mapping to the `XRES` symbol. |
| `-H`, `--height` | `integer` | Vertical resolution value mapping to the `YRES` symbol. |
| `--resolution` | `W` `H` | Alternate way to specify both width and height. |
| `--pass` | `name` | Pass name to process if the input is a project directory. |

---

## Baking Process

When preprocessing, Fork Eater runs the following sequence to build the final baked GLSL file:

### 1. Resolve Include Directives
Resolves `#pragma include(lib/noise.glsl)` and locally referenced shaders recursively, replacing them with their preprocessed source code contents.

### 2. Resolution Substitution
The symbols `iResolution` and `u_resolution` are replaced with static expressions using `XRES` and `YRES`:
*   `#define XRES <width>` and `#define YRES <height>` are injected at the top.
*   `uniform vec3 iResolution;` is replaced with `const vec3 iResolution = vec3(XRES, YRES, 1.0);`
*   `uniform vec2 u_resolution;` is replaced with `const vec2 u_resolution = vec2(XRES, YRES);`

This preserves any member access (e.g., `iResolution.xy`) without requiring manual expression replacement throughout the shader source.

### 3. Switches and Sliders Injection
*   If a switch declared via `#pragma switch(USE_BLUE)` is set to `true` in `uniforms.json`, `#define USE_BLUE` is injected at the top.
*   If a slider declared via `#pragma slider(ITERATIONS, ...)` is present, `#define ITERATIONS <value>` is injected at the top.

### 4. Uniform Variable Baking
All non-system, non-buffer uniform variables (i.e. not `iTime`, `u_time`, or `samplerBuffer`) are converted to constant variable initializers:
*   `uniform float brightness;` becomes `const float brightness = <value>;`
*   `uniform vec3 color;` becomes `const vec3 color = vec3(<r>, <g>, <b>);`

#### Value Lookup Priority:
1.  **JSON State**: Looks up the uniform value inside the project's `uniforms.json` file.
2.  **Pragma Default**: Looks for a default value declared in a `#pragma range(...)` or `#pragma slider(...)` directive.
3.  **Fallback**: Falls back to `0.0`.

---

## Example Invocation

```bash
# Preprocess a project and bake its uniforms
./fork-eater --preprocess project/my-effect -o baked.glsl -w 1920 -H 1080 --pass main
```
