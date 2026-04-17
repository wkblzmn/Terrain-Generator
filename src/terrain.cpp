#include "terrain.h"
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

Terrain::Terrain(int gridSize, int chunkVerts, float tileScale, float heightScale)
    : gridSize(gridSize)
    , chunkVerts(chunkVerts)
    , tileScale(tileScale)
    , heightScale(heightScale)
    , noise(42)
{}

Terrain::~Terrain() {
    for (auto& c : chunks) freeChunk(c);
}

// ---------------------------------------------------------------------------
// Height sampling
// ---------------------------------------------------------------------------

float Terrain::getHeight(float wx, float wz) const {
    // Map world coordinates into a [0,1]-ish noise domain
    float totalSize = gridSize * chunkVerts * tileScale;
    float nx = wx / totalSize;
    float nz = wz / totalSize;

    // 6-octave fBm, then redistribute with power curve for drama
    double h = noise.octave(nx * 3.5, nz * 3.5, 6, 0.5, 2.0);
    h = (h + 1.0) * 0.5;           // remap [-1,1] -> [0,1]
    h = std::pow(h, 1.6);          // push valleys down, keep peaks sharp
    return (float)(h * heightScale);
}

// ---------------------------------------------------------------------------
// Normal calculation (finite-difference central)
// ---------------------------------------------------------------------------

glm::vec3 Terrain::calcNormal(float wx, float wz, float step) const {
    float hL = getHeight(wx - step, wz);
    float hR = getHeight(wx + step, wz);
    float hD = getHeight(wx, wz - step);
    float hU = getHeight(wx, wz + step);
    // Cross product of tangent vectors gives surface normal
    return glm::normalize(glm::vec3(hL - hR, 2.0f * step, hD - hU));
}

// ---------------------------------------------------------------------------
// Chunk mesh construction
// ---------------------------------------------------------------------------

TerrainChunk Terrain::buildChunk(int cx, int cz, int lod) {
    TerrainChunk chunk;
    chunk.lod        = lod;
    chunk.worldOffset = glm::vec2(cx * chunkVerts * tileScale,
                                  cz * chunkVerts * tileScale);

    int   step      = 1 << lod;               // vertex step size: 1, 2, 4 …
    int   vside     = chunkVerts / step + 1;  // vertices per side at this LOD
    float ws        = tileScale * step;        // world distance between vertices

    std::vector<float>        verts;
    std::vector<unsigned int> idx;

    verts.reserve(vside * vside * 8);
    idx.reserve ((vside - 1) * (vside - 1) * 6);

    for (int z = 0; z < vside; ++z) {
        for (int x = 0; x < vside; ++x) {
            float wx = chunk.worldOffset.x + x * ws;
            float wz = chunk.worldOffset.y + z * ws;
            float h  = getHeight(wx, wz);
            glm::vec3 n = calcNormal(wx, wz, ws * 0.5f);

            // Position (3) + Normal (3) + TexCoord (2)
            verts.insert(verts.end(), { wx, h, wz });
            verts.insert(verts.end(), { n.x, n.y, n.z });
            verts.insert(verts.end(), {
                (float)x / (float)(vside - 1) * 10.0f,
                (float)z / (float)(vside - 1) * 10.0f
            });
        }
    }

    // Two triangles per quad, consistent winding
    for (int z = 0; z < vside - 1; ++z) {
        for (int x = 0; x < vside - 1; ++x) {
            unsigned int tl = z * vside + x;
            unsigned int tr = tl + 1;
            unsigned int bl = (z + 1) * vside + x;
            unsigned int br = bl + 1;
            idx.insert(idx.end(), { tl, bl, tr,  tr, bl, br });
        }
    }

    chunk.indexCount = (int)idx.size();

    glGenVertexArrays(1, &chunk.VAO);
    glGenBuffers(1, &chunk.VBO);
    glGenBuffers(1, &chunk.EBO);

    glBindVertexArray(chunk.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, chunk.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 idx.size() * sizeof(unsigned int),
                 idx.data(), GL_STATIC_DRAW);

    constexpr int stride = 8 * sizeof(float);
    // attrib 0: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // attrib 1: normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // attrib 2: texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return chunk;
}

void Terrain::freeChunk(TerrainChunk& chunk) {
    if (chunk.VAO) { glDeleteVertexArrays(1, &chunk.VAO); chunk.VAO = 0; }
    if (chunk.VBO) { glDeleteBuffers(1, &chunk.VBO);      chunk.VBO = 0; }
    if (chunk.EBO) { glDeleteBuffers(1, &chunk.EBO);      chunk.EBO = 0; }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Terrain::generateChunks() {
    chunks.reserve(gridSize * gridSize);
    for (int cz = 0; cz < gridSize; ++cz)
        for (int cx = 0; cx < gridSize; ++cx)
            chunks.push_back(buildChunk(cx, cz, 0));

    std::cout << "[Terrain] Generated " << chunks.size() << " chunks.\n";
}

void Terrain::update(const glm::vec3& camPos) {
    float chunkWorld = chunkVerts * tileScale; // world size of one chunk

    for (int i = 0; i < (int)chunks.size(); ++i) {
        auto& chunk = chunks[i];

        // Distance from camera to chunk centre
        glm::vec2 centre = chunk.worldOffset + glm::vec2(chunkWorld * 0.5f);
        float dist = glm::length(glm::vec2(camPos.x, camPos.z) - centre);

        // Pick LOD: 0 = close, 1 = medium, 2 = far
        int targetLOD;
        if      (dist < chunkWorld * 2.5f) targetLOD = 0;
        else if (dist < chunkWorld * 5.0f) targetLOD = 1;
        else                               targetLOD = 2;

        if (targetLOD != chunk.lod) {
            // Derive cx/cz from worldOffset
            int cx = (int)std::round(chunk.worldOffset.x / chunkWorld);
            int cz = (int)std::round(chunk.worldOffset.y / chunkWorld);
            freeChunk(chunk);
            chunks[i] = buildChunk(cx, cz, targetLOD);
        }
    }
}

void Terrain::render(Shader& shader) {
    shader.setFloat("heightScale", heightScale);
    for (auto& chunk : chunks) {
        glBindVertexArray(chunk.VAO);
        glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}
