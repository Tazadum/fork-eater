# Fork Eater Documentation

Welcome to the official documentation for Fork Eater. This documentation is designed to be easily parsed and updated by both humans and agentic AI.

For building and running on Linux or Windows, see the [project README](../README.md#prerequisites).

## Features

Below is a list of documented application features. Each file provides a detailed breakdown of its functionality, key code components, and configuration.

*   [Timeline FPS Indicator](./features/timeline-fps-indicator.md)
*   [Render Scaling](./features/render-scaling.md)
*   [Library Export](./features/library-export.md)
*   [Shader Pragmas and Parameters](./features/shader-pragmas.md)
*   [Data Buffer Support](./features/data-buffers.md)
*   [Camera System](./features/camera-system.md)
*   [Raymarching Libraries](./features/raymarching-libraries.md)
*   [Audio Playback and Sync](./features/audio-playback.md)
*   [Command-Line Preprocessor Mode](./features/preprocessor-cli.md)

## Templates

Fork Eater includes several templates to get you started quickly. These templates demonstrate best practices for using the application's features and libraries.

- **simple**: Minimal boilerplate, ideal for 2D effects.
- **music**: Example of music synchronization and beat-based animation.
- **mouse**: Interactive shader responding to mouse position.
- **raymarching**: Multi-pass example with Depth of Field using `raymarching_vec3.glsl`.
- **raymarching_basic**: Simple 3D scene using `raymarching.glsl`.
- **raymarching_vec2**: 3D scene with multiple materials using `raymarching_vec2.glsl`.

