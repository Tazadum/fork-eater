# Repository Guidelines

## Project Structure & Module Organization
Source files live in `src/` with one class per translation unit (for example `ShaderManager.cpp`, `MenuSystem.cpp`). Public headers mirror that layout in `include/`. Shader presets reside in `templates/` and are embedded at configure time; update them and rerun the build to regenerate `GeneratedShaderTemplates.h`. Shared GLSL snippets sit in `libs/`, while sample projects and assets are under `project/`. Third-party code (Dear ImGui, GLAD) belongs in `external/`. Keep generated artifacts in `build/`; avoid committing anything from that directory.

## Build, Test, and Development Commands
Use `./build.sh` from the repo root to configure CMake, clone Dear ImGui if needed, and compile the `fork-eater` binary into `build/`. On Windows with MSVC, use `.\build.ps1` (or `cmake --preset windows-msvc`); with MSYS2 MinGW, use `./build.sh` from the MINGW64 shell. For iterative work, `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` followed by `cmake --build build --parallel` keeps the cache intact. Run the editor via `./run.sh` / `.\run.ps1` or directly with `./build/fork-eater project/test` (Linux) or `.\build\fork-eater.exe project\test` (Windows). Pass `--help` for command-line options and `--test [exit_code]` when scripting smoke checks.

## Coding Style & Naming Conventions
The codebase targets C++17 with 4-space indentation and brace-on-same-line formatting. Match existing include ordering (standard library, third-party, project headers) and keep headers free of using directives. Classes use PascalCase, member functions camelCase, and member variables the `m_` prefix (`m_window`, `m_running`). File names remain snake_case. Prefer `std::unique_ptr`/`std::shared_ptr` to raw ownership and route user-facing logging through `Logger`.

## Testing Guidelines
There is no automated unit test suite yet; maintain the `test_exit.sh` (Linux) and `test_exit.ps1` (Windows) scripts as lightweight regression checks. After building, run `./test_exit.sh` or `.\test_exit.ps1` to confirm the binary responds correctly to `--help`, `--test`, and launch/termination scenarios. When adding new flags or startup flows, extend these scripts or add similar smoke tests beside them. Document any GPU-dependent testing steps in your pull request.

## Commit & Pull Request Guidelines
Follow the existing history by starting commit subjects with a capitalized action verb (`Add`, `Fix`, `Refactor`) and keeping them short. Group related changes together and explain shader/template updates in the body when they affect embedded assets. Pull requests should outline the feature or fix, note impacted shaders or runtime behaviour, and link issues where applicable. Include screenshots or short clips when the UI changes, and list manual test commands (e.g., `./test_exit.sh`, runtime scenarios) so reviewers can reproduce results quickly.
