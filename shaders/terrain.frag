#version 330 core

out vec4 FragColor;

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoord;
in float WorldHeight;

uniform vec3  lightDir;     // direction FROM scene TO light (i.e. -sunDirection)
uniform vec3  viewPos;
uniform float heightScale;
uniform vec3  fogColor;
uniform float fogDensity;

// ---------------------------------------------------------------------------
// Height-based colour palette
// t is in [0, 1] (WorldHeight / heightScale)
// ---------------------------------------------------------------------------
vec3 terrainColor(float t) {
    vec3 deepWater = vec3(0.04, 0.12, 0.32);
    vec3 shallows  = vec3(0.12, 0.38, 0.58);
    vec3 sand      = vec3(0.80, 0.73, 0.50);
    vec3 grass     = vec3(0.22, 0.50, 0.17);
    vec3 darkGrass = vec3(0.14, 0.34, 0.11);
    vec3 rock      = vec3(0.46, 0.42, 0.37);
    vec3 snow      = vec3(0.93, 0.96, 1.00);

    if (t < 0.04) return deepWater;
    if (t < 0.09) return mix(deepWater, shallows,  (t - 0.04) / 0.05);
    if (t < 0.14) return mix(shallows,  sand,       (t - 0.09) / 0.05);
    if (t < 0.22) return mix(sand,      grass,      (t - 0.14) / 0.08);
    if (t < 0.52) return mix(grass,     darkGrass,  (t - 0.22) / 0.30);
    if (t < 0.70) return mix(darkGrass, rock,       (t - 0.52) / 0.18);
    if (t < 0.85) return mix(rock,      snow,       (t - 0.70) / 0.15);
    return snow;
}

// ---------------------------------------------------------------------------
// Procedural detail: tile a tiny grid pattern to simulate texture without
// actual texture files. Cheap and looks decent at a distance.
// ---------------------------------------------------------------------------
float detailPattern(vec2 uv) {
    // Two overlapping sine waves → subtle variation
    return 0.5 + 0.5 * sin(uv.x * 6.28318) * sin(uv.y * 6.28318) * 0.08;
}

void main() {
    vec3 norm  = normalize(Normal);
    vec3 light = normalize(-lightDir);   // vector FROM fragment TOWARD light

    // ---- Blinn-Phong ----
    vec3  viewDir = normalize(viewPos - FragPos);
    vec3  halfVec = normalize(light + viewDir);

    float ambient  = 0.22;
    float diffuse  = max(dot(norm, light), 0.0);
    float specular = pow(max(dot(norm, halfVec), 0.0), 48.0) * 0.12;

    // ---- Base colour from height ----
    float t = clamp(WorldHeight / heightScale, 0.0, 1.0);
    vec3 baseColor = terrainColor(t);

    // ---- Slope-based rock overlay ----
    // norm.y close to 0 = near-vertical face → show rock regardless of height
    float slope = 1.0 - norm.y;
    vec3 slopeRock = vec3(0.40, 0.37, 0.33);
    baseColor = mix(baseColor, slopeRock, clamp(slope * 3.0 - 0.3, 0.0, 1.0));

    // ---- Subtle detail ----
    baseColor *= detailPattern(TexCoord);

    // ---- Combine lighting ----
    vec3 litColor = baseColor * (ambient + diffuse) + vec3(specular);

    // ---- Exponential squared fog ----
    float dist      = length(viewPos - FragPos);
    float fogFactor = exp(-(fogDensity * dist) * (fogDensity * dist));
    fogFactor       = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fogColor, litColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
