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

## How OpenGL Works in This Project

### 1. Initialization Chain (GLFW → GLAD → OpenGL)

Before any OpenGL function can be called, three things must happen in order:

**GLFW** creates the OS window and an OpenGL *context* — the state machine that OpenGL lives inside. Without it there is no GPU connection at all.
```cpp
glfwInit();
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);   // request OpenGL 3.3
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_SAMPLES, 4);                 // 4× MSAA
GLFWwindow* win = glfwCreateWindow(...);
glfwMakeContextCurrent(win);                     // bind context to this thread
```

**GLAD** then loads the actual OpenGL function pointers. On Windows, functions like `glDrawElements` live in the driver DLL and must be looked up at runtime. GLAD does all those `GetProcAddress` calls automatically:
```cpp
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
```
After this line every `gl*` call is a real pointer into your GPU driver.

Global OpenGL state is then configured:
```cpp
glEnable(GL_DEPTH_TEST);   // don't draw farther things in front of closer ones
glEnable(GL_MULTISAMPLE);  // activate the 4× MSAA requested from GLFW
glEnable(GL_CULL_FACE);    // skip back-facing triangles (performance)
```

---

### 2. GPU Memory — VAO, VBO, EBO

OpenGL doesn't accept a struct of vertices — you upload raw byte arrays to the GPU using three objects:

**VBO (Vertex Buffer Object)** — a blob of GPU memory holding vertex data. Each terrain vertex is 8 floats: `[x, y, z, nx, ny, nz, u, v]` (position + normal + texcoord).

**EBO (Element Buffer Object)** — a list of integer indices that define triangles. Instead of duplicating the 4 corners of every quad, they are stored once in the VBO and referenced by index: `[0,4,1, 1,4,5, ...]`.

**VAO (Vertex Array Object)** — the memory layout descriptor. It records *how* to interpret the bytes in the VBO: where position starts, how many floats, stride to the next vertex, etc. Binding the VAO at draw time is all that's needed.

In `terrain.cpp → buildChunk()`:
```cpp
glGenVertexArrays(1, &chunk.VAO);
glGenBuffers(1, &chunk.VBO);
glGenBuffers(1, &chunk.EBO);

glBindVertexArray(chunk.VAO);   // start recording layout

glBindBuffer(GL_ARRAY_BUFFER, chunk.VBO);
glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

// attrib 0 = position: 3 floats, stride = 8 floats, offset = 0
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// attrib 1 = normal: 3 floats, offset = 12 bytes
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
// attrib 2 = texcoord: 2 floats, offset = 24 bytes
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(2);
```

The slot numbers (0, 1, 2) match `layout(location = 0/1/2)` in `terrain.vert` — that is the bridge between CPU data and GPU shader input.

---

### 3. The Shader Pipeline

When `glDrawElements()` is called, the GPU runs vertex data through a fixed pipeline:

```
CPU vertices (VAO) → Vertex Shader → Rasterization → Fragment Shader → Screen
```

**Vertex Shader** (`terrain.vert`) runs once per vertex on the GPU. It transforms 3D world positions into 2D screen positions using the MVP matrices:
- **Model** — where the object sits in the world (identity here)
- **View** — the camera's perspective (built by `glm::lookAt`)
- **Projection** — perspective divide, turns 3D into 2D with depth (`glm::perspective`)

```glsl
vec4 worldPos   = model * vec4(aPos, 1.0);
gl_Position     = projection * view * worldPos;
```

It also passes `WorldHeight`, `Normal`, and `TexCoord` through to the fragment stage via `out` variables. These values are automatically interpolated across the triangle surface by the rasterizer.

**Fragment Shader** (`terrain.frag`) runs once per pixel covered by a triangle. It receives the interpolated values and outputs a final color:

1. **Biome color** — `terrainColor(WorldHeight / heightScale)` is a 7-stop piecewise gradient: deep water → shallows → sand → grass → dark grass → rock → snow. Each stop uses `mix()` for smooth blending.
2. **Slope rock overlay** — `norm.y` near zero means a near-vertical cliff face. Rock color is blended in regardless of height: `mix(baseColor, slopeRock, clamp(slope * 3.0 - 0.3, 0.0, 1.0))`.
3. **Detail pattern** — two overlapping sine waves on UV coordinates add micro-variation without any texture files.
4. **Blinn-Phong lighting**:
   - *Ambient*: constant 0.22 — nothing is ever pitch black
   - *Diffuse*: `dot(normal, lightDir)` — surfaces facing the sun are brighter
   - *Specular*: `dot(normal, halfVector)^48` — tight highlight
5. **Exponential² fog**: `exp(-(density * distance)²)` — softer falloff than linear fog, blended toward `fogColor` which matches the skybox horizon for seamless transition.

---

### 4. Uniforms — Sending Data from CPU to GPU Each Frame

Uniforms are how per-frame values (matrices, positions, lighting) are passed from C++ to shaders. The `Shader` class wraps this:

```cpp
// CPU side (main.cpp)
terrainShader.setMat4("projection", proj);
terrainShader.setVec3("lightDir",   lightDir);

// GPU side (terrain.frag)
uniform vec3  lightDir;
uniform mat4  projection;
```

Internally this calls `glGetUniformLocation` to find the variable by name, then `glUniform*` to upload the value. The terrain shader receives 8 uniforms per frame: `model`, `view`, `projection`, `viewPos`, `lightDir`, `fogColor`, `fogDensity`, and `heightScale`.

---

### 5. Depth Testing and the Skybox Trick

With `GL_DEPTH_TEST` enabled, OpenGL maintains a depth buffer — a per-pixel record of the closest fragment drawn so far. Fragments farther away are discarded automatically.

The skybox is a unit cube drawn last. If depth testing ran normally, the cube faces would be at depth ~1.0 and could be clipped by terrain. The fix is the `xyww` trick in `skybox.vert`:

```glsl
vec4 pos    = projection * view * vec4(aPos, 1.0);
gl_Position = pos.xyww;   // z = w, so after divide: depth = w/w = 1.0 (maximum)
```

This forces every skybox fragment to the maximum possible depth. Then in `skybox.h`:
```cpp
glDepthFunc(GL_LEQUAL);   // pass if depth <= stored (not just <)
// draw skybox
glDepthFunc(GL_LESS);     // restore default
```

`GL_LEQUAL` is needed because terrain at the horizon also writes depth = 1.0, and `GL_LESS` would reject the skybox there, leaving black holes.

---

### 6. How GLM is Used

GLM provides CPU-side math that mirrors GLSL syntax and uploads cleanly to OpenGL via `glm::value_ptr()`.

| Usage | Code |
|---|---|
| View matrix | `glm::lookAt(Position, Position + Front, Up)` |
| Projection matrix | `glm::perspective(glm::radians(fov), aspect, near, far)` |
| Strip translation for skybox | `glm::mat4(glm::mat3(view))` — discards the translation column |
| Normal matrix in shader | `mat3(transpose(inverse(model)))` — corrects normals under non-uniform scale |
| Upload to GPU | `glUniformMatrix4fv(..., glm::value_ptr(matrix))` |

---

### 7. The Render Loop — Frame Sequence

Every frame executes this exact sequence:

```
1. glfwPollEvents()         — process OS messages (mouse, keyboard, resize)
2. processInput()           — read key state, move camera, toggle wireframe
3. terrain.update(camPos)   — recompute LOD, rebuild any changed chunks
4. glClear(COLOR | DEPTH)   — wipe the framebuffer and depth buffer
5. terrainShader.use()      — bind terrain GPU program
   upload uniforms          — matrices, light, fog, etc.
   terrain.render()         — for each chunk: glBindVertexArray + glDrawElements
6. skyboxShader.use()       — bind skybox GPU program
   skybox.render()          — glDepthFunc(LEQUAL) → glDrawArrays → restore
7. glfwSwapBuffers()        — flip back/front buffer (shows the completed frame)
```

OpenGL renders to a hidden *back buffer*. `glfwSwapBuffers()` atomically swaps it with the visible *front buffer* — this is double buffering, which prevents screen tearing.

---

## Possible Extensions

- **Water plane** — flat quad at a fixed height with a framebuffer reflection
- **Shadow mapping** — render depth from sun, sample in terrain fragment shader
- **Tessellation shaders** — GPU-side LOD using `GL_PATCHES`
- **Texture splatting** — replace procedural colors with sampled textures blended by height
- **Day/night cycle** — animate `lightDir` and sky colors over time
- **Camera height lock** — clamp camera Y to `terrain.getHeight(x, z) + eyeHeight`
