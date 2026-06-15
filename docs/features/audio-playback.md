# Audio Playback and Sync

Fork Eater supports real-time, cross-platform MP3 audio playback. The audio player integrates directly with the project timeline to achieve absolute synchronization, enabling precise music-aligned shader animations and visuals.

## Features

- **MP3 Decoding & Playback**: Cross-platform support for MP3 decoding and audio playback on Linux, macOS, and Windows.
- **Absolute Timeline Sync**: When playing at normal speed (1.0x), the audio hardware clock serves as the master time source for the editor, ensuring the timeline and shaders never drift from the music.
- **Interactive Seeking**: Seeking the timeline (dragging the slider, jumping with shortcuts, looping, or resetting) immediately seeks the MP3 decoder to the corresponding audio frame.
- **Graceful Fallbacks**: If no audio file is specified, or if loading fails/audio devices are unavailable, the editor automatically falls back to frame-rate-based timeline update (`DeltaTime`), ensuring the application remains fully functional.

## Configuration

Audio files are specified in the project's manifest file (`4k-eater.project` or `4k-eater`) using the optional `"audio"` key with a file path relative to the project directory:

```json
{
  "name": "My Audio Project",
  "description": "An interactive shader aligned to music",
  "version": "1.0",
  "timelineLength": 60.0,
  "bpm": 128.0,
  "beatsPerBar": 4,
  "audio": "assets/music.mp3",
  "passes": [
    {
      "name": "main",
      "vertexShader": "basic.vert",
      "fragmentShader": "colorful.frag",
      "enabled": true
    }
  ]
}
```

## Architecture & Code Structure

The audio playback functionality is organized across the following files:

1.  **[AudioSystem.h](../../include/AudioSystem.h) & [AudioSystem.cpp](../../src/AudioSystem.cpp)**:
    -   Implements the core audio engine using the single-header [miniaudio](https://github.com/mackron/miniaudio) library.
    -   Exposes a clean C++ interface and hides miniaudio implementation macros/details via opaque pointers (`void* m_decoderPtr`, `void* m_devicePtr`).
    -   Handles decoder seeking and reads frames in a lock-free, thread-safe manner using atomic state variables.
2.  **[ShaderProject.h](../../include/ShaderProject.h) & [ShaderProject.cpp](../../src/ShaderProject.cpp)**:
    -   Parses and serializes the `"audio"` JSON property within the project manifest.
3.  **[ShaderEditor.h](../../include/ShaderEditor.h) & [ShaderEditor.cpp](../../src/ShaderEditor.cpp)**:
    -   Orchestrates the binding between the `Timeline` callbacks and the `AudioSystem`.
    -   During the main update loop, queries the `AudioSystem` cursor and updates the timeline `currentTime` if audio is playing.
    -   Handles custom seek requests via the `onTimeChanged` and `onPlayStateChanged` event hooks.

## Platform Support & Linking

The audio system is implemented using native backend drivers for low latency and zero external dependencies:
- **Windows**: WASAPI (Standard Windows Multimedia APIs).
- **Linux**: ALSA/PulseAudio (loaded dynamically at runtime via `dlopen`).
- **macOS**: CoreAudio/AudioToolbox (linked via CMake frameworks).

The build system automatically links target platforms to the appropriate frameworks:
```cmake
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
endif()
```
