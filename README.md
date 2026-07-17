# Nextcar

Code-first arcade racing prototype built with Unreal Engine and C++.

The first milestone is a lightweight drivable car on an effectively infinite flat test surface. The project deliberately avoids required Blueprint and content assets so the core gameplay systems remain reproducible from source.

## Prototype controls

- `W` / right trigger — accelerate
- `S` / left trigger — reverse or brake
- `A`, `D` / left stick — steer
- `Space` / gamepad face button bottom — handbrake
- `R` / gamepad face button top — reset the car

## Requirements

- Unreal Engine 5.8
- A C++ toolchain supported by the installed Unreal Engine version
- On Windows: Visual Studio 2022 with **Game development with C++** and Unreal Engine tools

## Running the project

1. Clone the repository.
2. Right-click `Nextcar.uproject` and select **Generate Visual Studio project files**.
3. Build the `NextcarEditor` target in the `Development Editor` configuration.
4. Open `Nextcar.uproject` and press **Play**.

The project uses Unreal's built-in `Template_Default` map and engine primitive meshes. The game mode creates the car and the recentering ground surface at runtime.

## Current architecture

- `AArcadeCarPawn` — deterministic, kinematic arcade movement and chase camera.
- `AInfiniteGround` — a large grid surface recentered around the player.
- `ANextcarGameMode` — runtime scene bootstrap and default pawn setup.
- `ANextcarHUD` — speed and control telemetry.

The vehicle movement is intentionally temporary. It will later be replaced by the dedicated chassis, powertrain, transmission, and procedural engine-audio modules.
