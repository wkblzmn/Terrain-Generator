# Procedural Terrain Generator

A real-time 3D procedural terrain renderer built with **OpenGL 3.3 Core**, **GLFW**, and **GLM**. The terrain is generated entirely on the CPU using Fractal Brownian Motion (fBm) Perlin noise and rendered with dynamic LOD (Level of Detail), Blinn-Phong lighting, height-based biome coloring, exponential fog, and a fully procedural skybox — no texture files required.

---

## Features

| Feature | Details |
|---|---|
| **Perlin fBm terrain** | 6-octave fractal Brownian motion, power-redistributed for dramatic peaks |
| **Height-based coloring** | Deep water → shallows → sand → grass → dark grass → rock → snow |
| **Slope blending** | Steep faces automatically show rock regardless of altitude |
| **Blinn-Phong lighting** | Ambient + diffuse + specular with a directional sun |
| **Exponential² fog** | Depth-based atmospheric haze that blends into the skybox color |
| **Procedural skybox** | Gradient sky, warm horizon glow, analytic cloud streaks, sun disc |
| **Chunk-based LOD** | 3 LOD levels (full, half, quarter res) selected by camera distance |
| **Free-fly camera** | WASD + mouse look + Space/Shift for up/down |
| **Wireframe toggle** | Press **F** to see the mesh |
| **MSAA 4×** | Enabled by default via GLFW |

---

## Controls

| Key / Input | Action |
|---|---|
| `W` `A` `S` `D` | Fly forward / left / backward / right |
| `Space` | Fly up |
| `Left Shift` | Fly down |
| Mouse | Look around |
| Scroll Wheel | Zoom (adjust FOV) |
| `F` | Toggle wireframe mode |
| `Esc` | Quit |

---

## Project Structure

```
Terrain Generator/
├── CMakeLists.txt          # Build system
├── shaders/
│   ├── terrain.vert        # Terrain vertex shader (MVP transform, passes height/normal)
│   ├── terrain.frag        # Terrain fragment shader (biome color, lighting, fog)
│   ├── skybox.vert         # Skybox vertex shader (xyww depth trick)
│   └── skybox.frag         # Skybox fragment shader (gradient, sun, clouds)
├── src/
│   ├── main.cpp            # Entry point, render loop, input handling
│   ├── terrain.h / .cpp    # Chunk generation, LOD system, height sampling
│   ├── noise.h             # Perlin noise + fBm implementation
│   ├── camera.h            # Free-fly camera (keyboard + mouse)
│   ├── shader.h            # GLSL shader loader/compiler wrapper
│   └── skybox.h            # Procedural skybox mesh + render
└── vendor/
    └── glad/               # OpenGL function loader (generated from glad.dav1d.de)
```

---

## Architecture Overview

### `main.cpp`
The application entry point. Initializes GLFW/GLAD, creates the window with 4× MSAA, registers input callbacks, constructs the `Terrain` and `Skybox` objects, and runs the main render loop. Each frame it:
1. Computes delta time and updates the FPS counter in the window title
2. Processes keyboard input for camera movement and wireframe toggle
3. Calls `terrain.update(cameraPos)` for dynamic LOD
4. Renders terrain, then skybox with a translation-stripped view matrix so the sky stays at infinity

### `noise.h` — `PerlinNoise`
A self-contained implementation of **Ken Perlin's improved noise** (2002). The permutation table is seeded with a random shuffle. The `octave()` method stacks multiple noise samples at increasing frequency and decreasing amplitude (**fBm**) and normalizes the result to approximately `[-1, 1]`.

### `terrain.h / terrain.cpp` — `Terrain` + `TerrainChunk`
The terrain is split into `gridSize × gridSize` chunks (default 8×8 = 64 chunks).

**Height sampling** (`getHeight`): maps world XZ into noise space, runs 6-octave fBm at frequency 3.5, remaps `[-1,1]` → `[0,1]`, then applies a power curve (`h^1.6`) to push valleys down and keep peaks sharp.

**Mesh building** (`buildChunk`): each chunk builds a grid of vertices with layout `[position(3), normal(3), texcoord(2)]` uploaded to a VAO/VBO/EBO. Normals are computed by finite-difference central differences on `getHeight`. At LOD 0 the step is 1 vertex; at LOD 1 it is 2; at LOD 2 it is 4 — halving vertex count each level.

**Dynamic LOD** (`update`): every frame, each chunk's distance to the camera center is measured. Thresholds at 2.5× and 5× the chunk world size select LOD 0/1/2. If the target LOD differs from the current one, the GPU buffers are freed and the chunk is rebuilt on the fly.

### `camera.h` — `Camera`
A standard free-fly camera. Keyboard input moves along `Front`, `Right`, and `WorldUp` vectors scaled by `MovementSpeed * deltaTime`. Mouse input updates `Yaw` and `Pitch` (clamped ±89°), which recomputes `Front`/`Right`/`Up` via trigonometry. Scroll adjusts the FOV (`Zoom`).

### `shader.h` — `Shader`
Loads vertex and fragment shader source from disk, compiles them, links the program, and exposes typed uniform setters (`setFloat`, `setVec3`, `setMat4`, etc.).

### `skybox.h` — `Skybox`
A unit cube (36 vertices) rendered with a translation-stripped view matrix. The vertex shader uses the `pos.xyww` trick to force NDC depth to 1.0, so the skybox always renders behind everything without a separate depth pass. The fragment shader computes a sky gradient from zenith to horizon, adds a warm glow and hard sun disc, and overlays cheap analytic cloud streaks using layered sine functions.

### `shaders/terrain.frag`
The main terrain fragment shader pipeline per-fragment:
1. **Biome color** via `terrainColor(WorldHeight / heightScale)` — 7-stop piecewise linear gradient
2. **Slope rock overlay** — blends in rock color when the surface normal leans away from vertical
3. **Detail pattern** — two overlapping sine waves on UV coordinates add subtle micro-variation without textures
4. **Blinn-Phong** — ambient (0.22) + diffuse + specular (exponent 48, weight 0.12)
5. **Exponential squared fog** — `exp(-(density * dist)²)`, blends toward `fogColor`

---

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| [GLFW](https://www.glfw.org/) | Window creation, OpenGL context, input | ≥ 3.3 |
| [GLM](https://github.com/g-truc/glm) | Math (vectors, matrices) | any recent |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader | OpenGL 3.3 Core (included in `vendor/`) |
| CMake | Build system | ≥ 3.14 |

---

## Building & Running

> **Important:** Always run the executable from the **project root directory** so that the relative paths `shaders/terrain.vert` etc. resolve correctly. On Windows, CMake's post-build step copies the shaders next to the `.exe` automatically, so running from `build/Release/` also works.

---

### Windows — vcpkg + MSVC (recommended)

```powershell
# 1. Install vcpkg (one-time setup)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# 2. Install dependencies
C:\vcpkg\vcpkg install glfw3 glm --triplet x64-windows

# 3. Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# 4. Build
cmake --build build --config Release

# 5. Run
.\build\Release\terrain.exe
```

---

### Windows — MSYS2 / MinGW

```bash
# 1. In an MSYS2 MinGW64 shell, install dependencies
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-glfw mingw-w64-x86_64-glm

# 2. Configure
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build

# 4. Run from project root
./build/terrain.exe
```

---

### Linux (Ubuntu / Debian)

```bash
# 1. Install dependencies
sudo apt install cmake build-essential libglfw3-dev libglm-dev

# 2. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build -j$(nproc)

# 4. Run from project root
./build/terrain
```

---

### Linux (Fedora / RHEL)

```bash
# 1. Install dependencies
sudo dnf install cmake gcc-c++ glfw-devel glm-devel

# 2. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build -j$(nproc)

# 4. Run
./build/terrain
```

---

### Linux (Arch)

```bash
# 1. Install dependencies
sudo pacman -S cmake glfw-x11 glm

# 2. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build -j$(nproc)

# 4. Run
./build/terrain
```

---

### macOS (Homebrew)

```bash
# 1. Install dependencies
brew install cmake glfw glm

# 2. Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build

# 4. Run
./build/terrain
```

> On Apple Silicon or if Xcode SDK is not found, add:
> `-DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)`

---

## Tuning Parameters

All major parameters are in `main.cpp` at the `Terrain` constructor call and shader uniform setup:

```cpp
// main.cpp — line ~108
Terrain terrain(
    8,      // gridSize   — chunks per axis (total = 8×8 = 64 chunks)
    32,     // chunkVerts — vertices per chunk side at full LOD
    4.0f,   // tileScale  — world distance between vertices (larger = wider terrain)
    80.0f   // heightScale — max terrain height in world units
);

// main.cpp — line ~121
terrainShader.setFloat("fogDensity", 0.0025f); // lower = clearer, higher = thicker fog
```

LOD distance thresholds (in `terrain.cpp` → `update()`):

```cpp
if      (dist < chunkWorld * 2.5f) targetLOD = 0;  // full resolution
else if (dist < chunkWorld * 5.0f) targetLOD = 1;  // half resolution
else                               targetLOD = 2;   // quarter resolution
```

Noise seed (in `terrain.cpp` constructor initializer):

```cpp
, noise(42)   // change to get a different world
```

---

## Possible Extensions

- **Water plane** — flat quad at a fixed height with a framebuffer reflection
- **Shadow mapping** — render depth from sun, sample in terrain fragment shader
- **Tessellation shaders** — GPU-side LOD using `GL_PATCHES`
- **Texture splatting** — replace procedural colors with sampled textures blended by height
- **Day/night cycle** — animate `lightDir` and sky colors over time
- **Camera height lock** — clamp camera Y to `terrain.getHeight(x, z) + eyeHeight`
