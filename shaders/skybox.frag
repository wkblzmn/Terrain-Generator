#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

void main() {
    vec3 dir = normalize(TexCoords);

    // ---------- Sky gradient ----------
    // t = 0 at horizon, 1 at zenith, negative below horizon
    float t = dir.y;  // [-1, 1]

    vec3 zenith   = vec3(0.08, 0.22, 0.62);   // deep sky blue
    vec3 horizon  = vec3(0.62, 0.78, 0.95);   // light hazy blue
    vec3 lowHaze  = vec3(0.75, 0.68, 0.55);   // warm horizon glow
    vec3 ground   = vec3(0.28, 0.24, 0.20);   // ground underside

    vec3 sky;
    if (t >= 0.0) {
        // Above horizon: horizon -> zenith
        float blend = pow(clamp(t, 0.0, 1.0), 0.5);
        sky = mix(horizon, zenith, blend);
    } else {
        // Below horizon: low haze -> ground
        float blend = clamp(-t * 4.0, 0.0, 1.0);
        sky = mix(lowHaze, ground, blend);
    }

    // ---------- Sun disc ----------
    // Sun direction must match lightDir sign convention in terrain.frag
    // lightDir in main.cpp is (-0.6, -0.9, 0.4) so the sun is at (0.6, 0.9, -0.4)
    vec3 sunDir = normalize(vec3(0.6, 0.9, -0.4));

    float sunDot = dot(dir, sunDir);

    // Soft glow around sun
    float glow = max(sunDot, 0.0);
    sky += vec3(0.6, 0.4, 0.1) * pow(glow, 12.0) * 0.4;

    // Hard sun disc
    if (sunDot > 0.9990) {
        float rim = (sunDot - 0.9990) / (1.0 - 0.9990);
        sky = mix(sky, vec3(1.0, 0.97, 0.82), rim);
    }

    // ---------- Simple "cloud" streaks via noise-like function ----------
    // Cheap analytic "clouds" — not real noise, just layered trig
    if (dir.y > 0.05) {
        float cx = dir.x / (dir.y + 0.01);
        float cz = dir.z / (dir.y + 0.01);
        float cloud = 0.5 + 0.5 * sin(cx * 1.7 + 0.8) * sin(cz * 1.3 + 1.1)
                         + 0.25 * sin(cx * 3.1 + 2.0) * sin(cz * 2.9 - 0.5);
        cloud = clamp((cloud - 0.55) * 5.0, 0.0, 1.0);
        // Fade clouds toward horizon
        cloud *= clamp((dir.y - 0.05) * 6.0, 0.0, 1.0);
        vec3 cloudColor = mix(vec3(0.90, 0.92, 0.95), vec3(1.0), 0.4);
        sky = mix(sky, cloudColor, cloud * 0.6);
    }

    FragColor = vec4(sky, 1.0);
}
