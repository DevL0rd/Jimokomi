# Jimokomi

Jimokomi now builds from the repository root as a C project while the Lua source stays frozen alongside it.

Current layout:
- `Engine/` contains the engine runtime, rendering, physics, and core systems
- `Game/` contains the playable sandbox and scene wiring
- `third_party/` vendors upstream dependencies such as Box2D and raylib

Build:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build -j`
- or use `./build.sh` on Unix-like shells
- or use `.\build.ps1` from PowerShell on Windows

Output:
- Unix-like builds produce `build/jimokomi`
- Windows builds produce `build/jimokomi.exe`
