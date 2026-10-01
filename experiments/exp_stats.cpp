// Eksperimentai 5-6: kolizijos ir lavina.
#include "common.h"
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>

// ---------- 5. Kolizijos ----------

// Rinkinio analizė: kolizija skaičiuojama tik skirtingoms įvestims.
static void analyse(Ctx& c, FILE* out, FILE* ex, const std::string& name, const std::vector<std::string>& inputs)
{
    std::set<std::string> uniq(inputs.begin(), inputs.end());
    std::unordered_map<std::string, std::vector<const std::string*>> byHash;
    for (auto& s : uniq)
    {
        Hasher h = c.H(toBytes(s));
        byHash[std::string(h.begin(), h.end())].push_back(&s);
    }
    long groups = 0, involved = 0;
    for (auto& [h, v] : byHash)
    {
        if (v.size() < 2) continue;
        ++groups;
        involved += v.size();
        if (groups <= 5)
        {
            fprintf(ex, "%s: hash=%s\n", name.c_str(), hexEsc(h).c_str());
            for (auto* s : v) fprintf(ex, "  ivestis(hex)=%s\n", hexEsc(*s).c_str());
        }
    }
    fprintf(out, "%s,%zu,%zu,%zu,%ld,%ld\n", name.c_str(), inputs.size(), uniq.size(), byHash.size(), groups, involved);
}

static std::string bytesStr(uint64_t v, int k)
{
    std::string s(k, 0);
    for (int i = 0; i < k; ++i) s[i] = char((v >> (8 * i)) & 0xFF);
    return s;
}

void expCollisions(Ctx& c)
{
    Rng r(SEED + 5);
    const size_t M = 100000;
    const int lens[4] = {10, 100, 500, 1000};

    // a) Poros
    FILE* fp = c.open("collisions_pairs.csv");
    fprintf(fp, "length,pairs,collisions\n");
    for (int n : lens)
    {
        long col = 0;
        for (size_t i = 0; i < M; ++i)
        {
            std::string a, b;
            do { a = randStr(r, n); b = randStr(r, n); } while (a == b);
            col += c.H(toBytes(a)) == c.H(toBytes(b));
        }
        fprintf(fp, "%d,%zu,%ld\n", n, M, col);
    }
    fclose(fp);

    // b) Visi rinkiniai ir struktūruoti atvejai
    FILE* fs = c.open("collisions_sets.csv");
    FILE* ex = c.open("collisions_examples.txt");
    fprintf(fs, "set,inputs,distinct_inputs,distinct_hashes,colliding_groups,inputs_in_groups\n");
    for (int n : lens)
    {
        std::vector<std::string> v;
        for (size_t i = 0; i < M; ++i) v.push_back(randStr(r, n));
        analyse(c, fs, ex, "random_len" + std::to_string(n), v);
    }
    {
        std::vector<std::string> v;
        std::string p = "abcdefgh";
        do { v.push_back(p); } while (std::next_permutation(p.begin(), p.end()));
        analyse(c, fs, ex, "perm_abcdefgh", v);
        v.clear();
        p = "Letuvai"; // 7 skirtingos raidės (be ąčė, tik ASCII)
        std::sort(p.begin(), p.end());
        do { v.push_back(p); } while (std::next_permutation(p.begin(), p.end()));
        analyse(c, fs, ex, "perm_7letters", v);
    }
    for (std::string pat : {"a", "ab", "abc"})
    {
        std::vector<std::string> v;
        for (int k = 1; k <= 3000; ++k)
        {
            std::string s;
            for (int i = 0; i < k; ++i) s += pat;
            v.push_back(s);
        }
        analyse(c, fs, ex, "repeat_" + pat, v);
    }
    {
        std::vector<std::string> v = {""};
        for (int b = 0; b < 256; ++b) v.push_back(bytesStr(b, 1));
        analyse(c, fs, ex, "all_0_1_byte", v);
        v.clear();
        for (uint64_t x = 0; x < 65536; ++x) v.push_back(bytesStr(x, 2));
        analyse(c, fs, ex, "all_2_bytes", v);
    }
    for (int k = 1; k <= 8; ++k) // besiskiria tik paskutiniai k baitai
    {
        std::string base = randStr(r, 100 - k);
        std::vector<std::string> v;
        if (k <= 2)
            for (uint64_t x = 0; x < (1ull << (8 * k)); ++x) v.push_back(base + bytesStr(x, k));
        else
            for (size_t i = 0; i < M; ++i) v.push_back(base + bytesStr(r.g(), k));
        analyse(c, fs, ex, "last_" + std::to_string(k) + "_bytes_differ", v);
    }
    {
        std::vector<std::string> v = {"", std::string(1, '\0')};
        for (int i = 0; i < 50000; ++i)
        {
            std::string x = randStr(r, 10);
            v.push_back(x);
            v.push_back(x + std::string(1, '\0'));
        }
        analyse(c, fs, ex, "x_vs_x_NUL", v);
    }
    fclose(fs);
    fclose(ex);

    // c) Nukirptos maišos: kiekviena juosta atskirai, palyginimas su gimtadienio įverčiu
    FILE* ft = c.open("collisions_truncated.csv");
    fprintf(ft, "length,bits,lane,m,colliding_pairs,expected_pairs\n");
    for (int n : {10, 100})
    {
        std::set<std::string> uniq;
        while (uniq.size() < M) uniq.insert(randStr(r, n));
        std::vector<Hasher> hs;
        for (auto& s : uniq) hs.push_back(c.H(toBytes(s)));
        for (int bits : {24, 32})
            for (int j = 0; j < 8; ++j)
            {
                std::unordered_map<uint32_t, long> cnt;
                for (auto& h : hs) ++cnt[lane(h, j) >> (32 - bits)];
                double pairs = 0;
                for (auto& [k, v] : cnt) pairs += (double)v * (v - 1) / 2;
                double m = (double)hs.size();
                fprintf(ft, "%d,%d,%d,%zu,%.0f,%.2f\n", n, bits, j, hs.size(), pairs, m * (m - 1) / 2 * std::pow(2.0, -bits));
            }
    }
    fclose(ft);
}

// ---------- 6. Lavina ----------

struct Acc
{
    Stat bits, hex;
    void add(const Hasher& a, const Hasher& b)
    {
        bits.add(bitDiff(a, b) * 100.0 / 256);
        hex.add(hexDiff(a, b) * 100.0 / 64);
    }
    void row(FILE* f, const std::string& label) const
    {
        fprintf(f, "%s,%ld,%.2f,%.2f,%.4f,%.2f,%.2f,%.4f\n", label.c_str(), bits.n, bits.mn, bits.mx, bits.avg(), hex.mn, hex.mx, hex.avg());
    }
};
static const char* HDR = "group,pairs,bits_min_pct,bits_max_pct,bits_avg_pct,hex_min_pct,hex_max_pct,hex_avg_pct\n";

// Vienos pozicijos keitimas (ilgis nesikeičia).
static void changeAt(Ctx& c, Rng& r, size_t n, size_t pos, Acc& acc, std::vector<long>* hist = nullptr)
{
    std::string s = randStr(r, n), t = s;
    t[pos] = otherChar(r, t[pos]);
    Hasher a = c.H(toBytes(s)), b = c.H(toBytes(t));
    acc.add(a, b);
    if (hist) ++(*hist)[bitDiff(a, b)];
}

void expAvalanche(Ctx& c)
{
    Rng r(SEED + 6);
    const int lens[4] = {10, 100, 500, 1000};
    const int P = 25000;

    // Pagrindinė: 4 x 25000 porų + histograma
    FILE* f = c.open("avalanche.csv");
    fprintf(f, HDR);
    Acc total;
    std::vector<long> histAll(257, 0);
    std::vector<std::vector<long>> histLen(4, std::vector<long>(257, 0));
    for (int li = 0; li < 4; ++li)
    {
        Acc a;
        for (int i = 0; i < P; ++i)
        {
            size_t n = lens[li];
            std::string s = randStr(r, n), t = s;
            size_t pos = r.below(n);
            t[pos] = otherChar(r, t[pos]);
            Hasher x = c.H(toBytes(s)), y = c.H(toBytes(t));
            a.add(x, y);
            total.add(x, y);
            ++histAll[bitDiff(x, y)];
            ++histLen[li][bitDiff(x, y)];
        }
        a.row(f, "len" + std::to_string(lens[li]));
    }
    total.row(f, "all");
    fclose(f);
    FILE* fh = c.open("avalanche_hist.csv");
    fprintf(fh, "bits,all,len10,len100,len500,len1000\n");
    for (int b = 0; b <= 256; ++b)
        fprintf(fh, "%d,%ld,%ld,%ld,%ld,%ld\n", b, histAll[b], histLen[0][b], histLen[1][b], histLen[2][b], histLen[3][b]);
    fclose(fh);

    // Pagal poziciją: pradžia / vidurys / paskutiniai 8 baitai
    FILE* fp = c.open("avalanche_position.csv");
    fprintf(fp, "length,position,");
    fprintf(fp, HDR);
    for (int n : lens)
        for (int cat = 0; cat < 3; ++cat)
        {
            Acc a;
            for (int i = 0; i < P; ++i)
            {
                size_t pos = cat == 0 ? r.below(8) : cat == 1 ? n / 2 - 4 + r.below(8) : n - 8 + r.below(8);
                changeAt(c, r, n, pos, a);
            }
            const char* nm[3] = {"first8", "middle8", "last8"};
            fprintf(fp, "%d,%s,", n, nm[cat]);
            a.row(fp, nm[cat]);
        }
    fclose(fp);

    // Atstumas nuo pabaigos k = 0..15 (H1)
    FILE* fe = c.open("avalanche_from_end.csv");
    fprintf(fe, "length,k_from_end,");
    fprintf(fe, HDR);
    for (int n : {10, 100})
        for (int k = 0; k < 16 && k < n; ++k)
        {
            Acc a;
            for (int i = 0; i < 5000; ++i) changeAt(c, r, n, n - 1 - k, a);
            fprintf(fe, "%d,%d,", n, k);
            a.row(fe, "k" + std::to_string(k));
        }
    fclose(fe);

    // Vieno bito apvertimas (baitų režimas)
    FILE* fb = c.open("avalanche_bitflip.csv");
    fprintf(fb, HDR);
    for (int n : lens)
    {
        Acc a;
        for (int i = 0; i < P; ++i)
        {
            Bytes s(n);
            for (auto& x : s) x = uint8_t(r.below(256));
            Bytes t = s;
            t[r.below(n)] ^= uint8_t(1u << r.below(8));
            a.add(c.H(s), c.H(t));
        }
        a.row(fb, "len" + std::to_string(n));
    }
    fclose(fb);

    // Trumpos įvestys: lavina ir kiek juostų dar lygios IV (H2)
    static const uint32_t IV[8] = {0x1A2B3C4Du, 0x5E6F7081u, 0x92A3B4C5u, 0xD6E7F809u, 0x1B2C3D4Eu, 0x5F607182u, 0x93A4B5C6u, 0xD7E8F90Au};
    FILE* fs = c.open("short_inputs.csv");
    fprintf(fs, "length,avg_lanes_equal_IV,bits_min_pct,bits_max_pct,bits_avg_pct\n");
    for (int n = 0; n <= 16; ++n)
    {
        double iv = 0;
        Acc a;
        for (int i = 0; i < 5000; ++i)
        {
            Hasher h = c.H(toBytes(randStr(r, n)));
            for (int j = 0; j < 8; ++j) iv += lane(h, j) == IV[j];
            if (n > 0) changeAt(c, r, n, r.below(n), a);
        }
        fprintf(fs, "%d,%.3f,%.2f,%.2f,%.4f\n", n, iv / 5000, n ? a.bits.mn : 0.0, n ? a.bits.mx : 0.0, n ? a.bits.avg() : 0.0);
    }
    fclose(fs);
}
