# Data Buffer Support

Fork Eater supports passing large structured datasets to shaders through the project manifest. These buffers can be defined in-line or loaded from external CSV files, and are automatically bound as either Texture Buffer Objects (TBOs) or Uniform Arrays.

## Manifest Configuration

Buffers are defined in the `buffers` section of the `4k-eater.project` manifest file:

```json
{
  "name": "My Project",
  "buffers": [
    {
      "name": "u_colors",
      "type": "vec3",
      "file": "assets/palette.csv"
    },
    {
      "name": "u_config",
      "type": "float",
      "data": [1.0, 0.5, 0.2]
    }
  ],
  "passes": [ ... ]
}
```

### Buffer Properties

- **`name`**: (String) The uniform name in the shader (e.g., `u_colors`).
- **`type`**: (String) The data type. Supported: `float`, `vec2`, `vec3`, `vec4`.
- **`file`**: (Optional, String) Path to an external CSV or whitespace-separated data file (relative to project root).
- **`data`**: (Optional, Array) In-line array of floating-point values.

> [!NOTE]
> If both `file` and `data` are provided, the `file` takes precedence.

## Shader Implementation

### Using Texture Buffer Objects (TBOs) - Recommended for Large Data
TBOs allow for millions of elements and are highly efficient. Use the `samplerBuffer` uniform type and `texelFetch` to access the data.

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform samplerBuffer u_colors;

void main() {
    // Fetch the 3rd color from the buffer (index 2)
    vec3 color = texelFetch(u_colors, 2).rgb;
    FragColor = vec4(color, 1.0);
}
```

### Using Uniform Arrays - For Small, Fixed Data
For smaller datasets, you can use standard GLSL uniform arrays. Fork Eater will automatically detect the array and populate it if the name matches a manifest buffer.

```glsl
#version 330 core
out vec4 FragColor;

uniform float u_config[3];

void main() {
    float param = u_config[0];
    FragColor = vec4(vec3(param), 1.0);
}
```

## CSV File Format

The CSV parser is lightweight and supports:
- Comma-separated values (`,`)
- Space-separated values
- Tab-separated values
- Comments starting with `#`
- Multiple values per line (automatically flattened)

Example `assets/palette.csv`:
```csv
# RGB Palette
1.0, 0.0, 0.0
0.0, 1.0, 0.0
0.0, 0.0, 1.0
```

## Hot-Reloading

Fork Eater monitors all buffer files specified in the manifest. Modifying and saving an external CSV file will trigger an automatic project reload, instantly updating your shader with the new data.

## Key Implementation Details

- **Binding Strategy**: TBOs are bound to texture units starting from unit 10 to avoid conflicts with shader pass inputs.
- **Resource Management**: `ShaderManager` automatically handles the creation, update, and deletion of OpenGL buffer and texture objects.
- **Data Validation**: The system logs the number of values loaded and reports errors if files are missing.
