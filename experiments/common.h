// Bendri eksperimentų įrankiai: RNG, abėcėlė, matavimai, CSV.
#pragma once
#include "../muhhash.h"
#include "../versions/algo.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;

constexpr uint64_t SEED = 20240929; // kiekvienas eksperimentas: SEED + jo numeris

// Deterministinis generatorius; savas atmetimo metodas vietoj uniform_int_distribution.
struct Rng
{
    std::mt19937_64 g;
    explicit Rng(uint64_t s) : g(s) {}
    uint64_t below(uint64_t n)
    {
        uint64_t lim = (UINT64_MAX / n) * n; // didžiausias n kartotinis
        uint64_t x;
        do { x = g(); } while (x >= lim);
        return x % n;
    }
};

// Abėcėlė: spausdinami ASCII 0x21..0x7E (94 simboliai).
constexpr int ALPHA = 94;
inline char alphaChar(uint64_t i) { return static_cast<char>(0x21 + i); }

inline std::string randStr(Rng& r, size_t n)
{
    std::string s(n, ' ');
    for (auto& c : s) c = alphaChar(r.below(ALPHA));
    return s;
}

// Kitas abėcėlės simbolis nei c.
inline char otherChar(Rng& r, char c)
{
    uint64_t i = r.below(ALPHA - 1);
    if (static_cast<int>(i) >= c - 0x21) ++i;
    return alphaChar(i);
}

inline Bytes toBytes(const std::string& s) { return Bytes(s.begin(), s.end()); }

// Vieta rezultatams ir pasirinktas algoritmas.
struct Ctx
{
    std::string algo = "v0.1";
    std::string dataDir = "data";
    std::string outDir = "results";
    std::string konst = "konstitucija.txt";
    HashFn H = nullptr;

    std::string dir() const
    {
        std::string d = algo;
        std::replace(d.begin(), d.end(), '.', '_');
        return outDir + "/" + d;
    }
    FILE* open(const std::string& name) const
    {
        std::filesystem::create_directories(dir());
        std::string p = dir() + "/" + name;
        FILE* f = fopen(p.c_str(), "w");
        if (!f) { perror(p.c_str()); exit(1); }
        return f;
    }
};

inline uint32_t lane(const Hasher& h, int j)
{
    return (uint32_t(h[j * 4]) << 24) | (uint32_t(h[j * 4 + 1]) << 16) | (uint32_t(h[j * 4 + 2]) << 8) | h[j * 4 + 3];
}

inline int bitDiff(const Hasher& a, const Hasher& b)
{
    int d = 0;
    for (int i = 0; i < 32; ++i) d += __builtin_popcount(a[i] ^ b[i]);
    return d;
}

// Besiskiriančios hex pozicijos (iš 64).
inline int hexDiff(const Hasher& a, const Hasher& b)
{
    std::string x = toHex(a), y = toHex(b);
    int d = 0;
    for (int i = 0; i < 64; ++i) d += x[i] != y[i];
    return d;
}

// Min/max/vidurkis.
struct Stat
{
    double mn = 1e18, mx = -1e18, sum = 0;
    long n = 0;
    void add(double v) { mn = std::min(mn, v); mx = std::max(mx, v); sum += v; ++n; }
    double avg() const { return n ? sum / n : 0; }
};

inline std::string hexEsc(const std::string& s)
{
    static const char* h = "0123456789abcdef";
    std::string o;
    for (unsigned char c : s) { o += h[c >> 4]; o += h[c & 15]; }
    return o.empty() ? "-" : o;
}

void expInputs(Ctx&);
void expFormat(Ctx&);
void expDeterminism(Ctx&);
void expSpeed(Ctx&);
void expCollisions(Ctx&);
void expAvalanche(Ctx&);
void expGuess(Ctx&);
void expRotations(Ctx&);
