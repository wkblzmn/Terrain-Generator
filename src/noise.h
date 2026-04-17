#pragma once
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>
#include <cmath>

// Classic Perlin Noise with Fractal Brownian Motion (fBm) support
class PerlinNoise {
    std::vector<int> p;

public:
    PerlinNoise(unsigned int seed = 42) {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);
        p.insert(p.end(), p.begin(), p.end()); // duplicate for overflow safety
    }

    // Single-octave 3D Perlin noise, returns [-1, 1]
    double noise(double x, double y, double z) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        int Z = (int)std::floor(z) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        double u = fade(x), v = fade(y), w = fade(z);

        int A  = p[X]   + Y, AA = p[A] + Z, AB = p[A+1] + Z;
        int B  = p[X+1] + Y, BA = p[B] + Z, BB = p[B+1] + Z;

        return lerp(w,
                lerp(v, lerp(u, grad(p[AA],   x,   y,   z  ),
                                grad(p[BA],   x-1, y,   z  )),
                        lerp(u, grad(p[AB],   x,   y-1, z  ),
                                grad(p[BB],   x-1, y-1, z  ))),
                lerp(v, lerp(u, grad(p[AA+1], x,   y,   z-1),
                                grad(p[BA+1], x-1, y,   z-1)),
                        lerp(u, grad(p[AB+1], x,   y-1, z-1),
                                grad(p[BB+1], x-1, y-1, z-1))));
    }

    // Fractal Brownian Motion — layers of noise at increasing frequency/decreasing amplitude
    // Returns approx [-1, 1], normalized by max possible value
    double octave(double x, double y, int octaves,
                  double persistence = 0.5, double lacunarity = 2.0) const {
        double total     = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double maxValue  = 0.0;

        for (int i = 0; i < octaves; i++) {
            total    += noise(x * frequency, 0.0, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude  *= persistence;
            frequency  *= lacunarity;
        }
        return total / maxValue;
    }

private:
    double fade(double t) const {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); // Ken Perlin's improved fade
    }
    double lerp(double t, double a, double b) const {
        return a + t * (b - a);
    }
    double grad(int hash, double x, double y, double z) const {
        int    h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
};
