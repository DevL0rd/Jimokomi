# Jimokomi

Jimokomi now builds from the repository root as a C project while the Lua source stays frozen alongside it.

Current layout:
- `Engine/` contains the engine runtime, rendering, physics, and core systems
- `Game/` contains the playable sandbox and scene wiring
- `third_party/` vendors upstream dependencies such as CorePhys and raylib
