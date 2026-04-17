# Procedural Terrain — OpenGL 3.3

A procedurally generated infinite-feeling terrain built with OpenGL 3.3 Core Profile.

## Features

| Feature | Details |
|---|---|
| **Perlin fBm terrain** | 6-octave fractal Brownian motion, power-redistributed for dramatic peaks |
| **Height-based colouring** | Deep water → shallows → sand → grass → dark grass → rock → snow |
| **Slope blending** | Steep faces automatically show rock regardless of altitude |
| **Blinn-Phong lighting** | Ambient + diffuse + specular with a directional sun |
| **Exponential² fog** | Depth-based atmospheric haze that blends into the skybox colour |
| **Procedural skybox** | Gradient sky, warm horizon glow, analytic "cloud" streaks, sun disc |
| **Chunk-based LOD** | 3 LOD levels (full, half, quarter res) selected by camera distance |
| **Freecam** | WASD + mouse look + Space/Shift for up/down |
| **Wireframe toggle** | Press **F** to see the mesh |
| **MSAA 4×** | Enabled by default via GLFW |

## Controls

| Key | Action |
|---|---|
| `W A S D` | Fly forward / left / back / right |
| `Space` | Fly up |
| `Left Shift` | Fly down |
| `Mouse` | Look around |
| `Scroll wheel` | Zoom FOV |
| `F` | Toggle wireframe |
| `ESC` | Quit |

---

## Prerequisites

Install with your package manager:

```bash
# Ubuntu / Debian
sudo apt install cmake build-essential libglfw3-dev libglm-dev

# Arch
sudo pacman -S cmake glfw-x11 glm

# macOS (Homebrew)
brew install cmake glfw glm
```

### GLAD (required — not included)

GLAD is a GL loader generated specifically for your platform.

1. Go to **https://glad.dav1d.de/**
2. Set:
   - Language: **C**
   - Specification: **OpenGL**
   - Profile: **Core**
   - API gl: **Version 3.3**
   - ✅ **Generate a loader**
3. Click **Generate** and download the zip
4. Extract and copy into this project:

```
vendor/
  glad/
    include/
      glad/glad.h
      KHR/khrplatform.h
    src/
      glad.c
```

---

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./terrain
```

On macOS you may need:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)
```

---

## Project Structure

```
terrain-project/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp        — window, render loop, input
│   ├── shader.h        — GLSL shader loader/wrapper
│   ├── camera.h        — free-flight camera
│   ├── noise.h         — Perlin noise + fBm
│   ├── terrain.h       — chunk-based terrain (header)
│   └── terrain.cpp     — terrain mesh generation + LOD
├── shaders/
│   ├── terrain.vert
│   ├── terrain.frag    — height colour, Blinn-Phong, fog
│   ├── skybox.vert
│   └── skybox.frag     — gradient sky, sun, analytic clouds
└── vendor/
    └── glad/           — (you download and place this)
```

---

## Tuning Parameters

All major parameters are at the top of `main.cpp` and in `Terrain` construction:

```cpp
// In main.cpp
Terrain terrain(
    8,      // gridSize  — chunks per axis (total = 8×8 = 64 chunks)
    32,     // chunkVerts— vertices per chunk side at full LOD
    4.0f,   // tileScale — world distance between verts (increase for wider terrain)
    80.0f   // heightScale — max terrain height
);

terrainShader.setFloat("fogDensity", 0.0025f); // lower = clearer day
```

### LOD thresholds (in `terrain.cpp → update()`)

```cpp
if      (dist < chunkWorld * 2.5f) targetLOD = 0;  // full res
else if (dist < chunkWorld * 5.0f) targetLOD = 1;  // half res
else                               targetLOD = 2;   // quarter res
```

---

## Possible Extensions (extra credit ideas)

- **Water plane** — flat quad at a fixed height with a framebuffer reflection
- **Shadow mapping** — render depth from sun, sample in terrain shader
- **Tessellation shaders** — GPU-side LOD using GL_PATCHES
- **Texture splatting** — replace procedural colours with sampled textures blended by height
- **Day/night cycle** — animate `lightDir` and sky colours over time
- **Collision / height lock** — clamp camera Y to `terrain.getHeight(x,z) + eyeHeight`
