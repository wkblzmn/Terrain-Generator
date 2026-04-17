#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "noise.h"
#include "shader.h"

// One GPU-resident mesh chunk of the terrain
struct TerrainChunk {
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int          indexCount = 0;
    glm::vec2    worldOffset{0.0f};   // bottom-left corner in world XZ
    int          lod = 0;             // 0 = full res, 1 = half, 2 = quarter
};

class Terrain {
public:
    // gridSize  : number of chunks along each axis
    // chunkVerts: vertices per chunk side at LOD 0 (must be power-of-2 + 1 friendly, e.g. 32)
    // tileScale : world-space distance between adjacent vertices at LOD 0
    // heightScale: maximum terrain height in world units
    Terrain(int gridSize   = 8,
            int chunkVerts = 32,
            float tileScale   = 4.0f,
            float heightScale = 80.0f);

    ~Terrain();

    // Generate all chunks at LOD 0
    void generateChunks();

    // Re-build chunks whose LOD should change given camera position
    void update(const glm::vec3& cameraPos);

    // Draw all chunks (shader must already be bound with MVP uniforms set)
    void render(Shader& shader);

    // Sample terrain height at world XZ (useful for camera clamping etc.)
    float getHeight(float worldX, float worldZ) const;

    float heightScale;

private:
    int   gridSize;
    int   chunkVerts;
    float tileScale;

    PerlinNoise           noise;
    std::vector<TerrainChunk> chunks;

    TerrainChunk buildChunk(int cx, int cz, int lod);
    void         freeChunk (TerrainChunk& chunk);
    glm::vec3    calcNormal(float wx, float wz, float step) const;
};
