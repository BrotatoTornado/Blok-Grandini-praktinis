// Eksperimentai 1-4: įvestys, formatas, determinizmas, sparta (+ pasukimų analizė).
#include "common.h"
#include <fstream>
#include <map>
#include <cmath>

namespace fs = std::filesystem;

static size_t utf8Chars(const Bytes& b)
{
    size_t n = 0;
    for (uint8_t c : b) n += (c & 0xC0) != 0x80;
    return n;
}

static std::vector<std::string> dataFiles(const Ctx& c)
{
    std::vector<std::string> v;
    for (auto& e : fs::directory_iterator(c.dataDir))
        if (e.is_regular_file()) v.push_back(e.path().filename().string());
    std::sort(v.begin(), v.end());
    return v;
}

// 1. Įvestys: sugeneruoja data/ failus ir jų maišas.
void expInputs(Ctx& c)
{
    Rng r(SEED + 1);
    fs::create_directories(c.dataDir);
    std::vector<std::pair<std::string, std::string>> in;
    in.push_back({"empty.txt", ""});
    in.push_back({"a.txt", "a"});
    in.push_back({"b.txt", "b"});
    size_t sizes[3] = {1500, 10000, 50000};
    for (int k = 0; k < 3; ++k)
    {
        std::string base = randStr(r, sizes[k]);
        std::string nm = "rand" + std::to_string(k + 1);
        in.push_back({nm + ".txt", base});
        size_t pos[3] = {0, base.size() / 2, base.size() - 1};
        const char* tag[3] = {"first", "mid", "last"};
        for (int p = 0; p < 3; ++p)
        {
            std::string m = base;
            m[pos[p]] = otherChar(r, m[pos[p]]);
            in.push_back({nm + "_" + tag[p] + ".txt", m});
        }
    }
    for (size_t n : {1, 2, 7, 8, 9, 16, 100, 1000, 10000})
        in.push_back({"rep_a_" + std::to_string(n) + ".txt", std::string(n, 'a')});
    in.push_back({"perm_abc.txt", "abc"});
    in.push_back({"perm_acb.txt", "acb"});
    in.push_back({"perm_bca.txt", "bca"});
    in.push_back({"space_lead.txt", " Lietuva"});
    in.push_back({"space_trail.txt", "Lietuva "});
    in.push_back({"lt_nonl.txt", "Lietuva"});
    in.push_back({"lt_nl.txt", "Lietuva\n"});
    in.push_back({"crlf.txt", "abc\r\n"});
    in.push_back({"lf.txt", "abc\n"});
    in.push_back({"lt_lower.txt", "lietuva"});
    in.push_back({"lt_cap.txt", "Lietuva"});
    in.push_back({"lt_cap_excl.txt", "Lietuva!"});
    in.push_back({"lt_diacritics.txt", "ąčęėįšųūž"});
    in.push_back({"emoji.txt", "😀🎉"});

    FILE* f = c.open("inputs.csv");
    fprintf(f, "name,chars,bytes,hash\n");
    for (auto& [name, content] : in)
    {
        std::ofstream(c.dataDir + "/" + name, std::ios::binary) << content;
        Bytes b = readFile(c.dataDir + "/" + name); // hash iš disko
        fprintf(f, "%s,%zu,%zu,%s\n", name.c_str(), utf8Chars(b), b.size(), toHex(c.H(b)).c_str());
    }
    fclose(f);
    printf("Irasyta %zu ivesciu\n", in.size());
}

// 2. Formatas: 64 hex simboliai; ieškoma maiša su pradiniais nuliais.
void expFormat(Ctx& c)
{
    FILE* f = c.open("format.csv");
    fprintf(f, "name,len,only_hex_lower\n");
    int bad = 0;
    for (auto& n : dataFiles(c))
    {
        std::string h = toHex(c.H(readFile(c.dataDir + "/" + n)));
        bool ok = h.size() == 64 && h.find_first_not_of("0123456789abcdef") == std::string::npos;
        bad += !ok;
        fprintf(f, "%s,%zu,%d\n", n.c_str(), h.size(), ok);
    }
    fclose(f);

    FILE* g = c.open("format_zeros.csv");
    fprintf(g, "zeros,trials,input,hash\n");
    Rng r(SEED + 2);
    for (int z = 1; z <= 3; ++z)
    {
        std::string pre(z, '0');
        for (long t = 1; t <= 5000000; ++t)
        {
            std::string s = randStr(r, 10);
            std::string h = toHex(c.H(toBytes(s)));
            if (h.compare(0, z, pre) == 0)
            {
                fprintf(g, "%d,%ld,%s,%s\n", z, t, s.c_str(), h.c_str());
                break;
            }
        }
    }
    fclose(g);
    printf("Formato klaidu: %d\n", bad);
}

// 3. Determinizmas procese: 5 kvietimai ir seka A,B,A.
void expDeterminism(Ctx& c)
{
    auto files = dataFiles(c);
    FILE* f = c.open("determinism.csv");
    fprintf(f, "name,repeat5_same,aba_ok\n");
    int bad = 0;
    for (size_t i = 0; i < files.size(); ++i)
    {
        Bytes a = readFile(c.dataDir + "/" + files[i]);
        Bytes b = readFile(c.dataDir + "/" + files[(i + 1) % files.size()]);
        Hasher h0 = c.H(a);
        bool same = true;
        for (int k = 0; k < 5; ++k) same = same && c.H(a) == h0;
        Hasher a1 = c.H(a);
        (void)c.H(b);
        bool aba = c.H(a) == a1;
        bad += !(same && aba);
        fprintf(f, "%s,%d,%d\n", files[i].c_str(), same, aba);
    }
    fclose(f);
    printf("Determinizmo klaidu: %d\n", bad);
}

// 4. Sparta: konstitucija.txt ištraukos 1,2,4,... eilučių ir visas failas.
void expSpeed(Ctx& c)
{
    Bytes all = readFile(c.konst);
    std::vector<size_t> ends; // eilučių pabaigos (po '\n')
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i] == '\n') ends.push_back(i + 1);
    if (ends.empty() || ends.back() != all.size()) ends.push_back(all.size());

    std::vector<size_t> lines;
    for (size_t n = 1; n < ends.size(); n *= 2) lines.push_back(n);
    lines.push_back(ends.size()); // visas failas

    const int WARM = 20, REPS = 10;
    const double MIN_NS = 5e6; // grupė ne trumpesnė nei ~5 ms
    volatile uint8_t sink = 0;
    FILE* raw = c.open("speed_raw.csv");
    FILE* sum = c.open("speed.csv");
    fprintf(raw, "lines,bytes,rep,calls,ns_per_hash\n");
    fprintf(sum, "lines,bytes,mean_ns,min_ns,max_ns,std_ns,ns_per_byte\n");
    using clk = std::chrono::steady_clock;
    for (size_t ln : lines)
    {
        Bytes buf(all.begin(), all.begin() + ends[ln - 1]);
        for (int w = 0; w < WARM; ++w) sink ^= c.H(buf)[0];
        auto t0 = clk::now();
        for (int k = 0; k < 10; ++k) sink ^= c.H(buf)[0];
        double est = std::chrono::duration<double, std::nano>(clk::now() - t0).count() / 10;
        long calls = std::max(1L, (long)(MIN_NS / std::max(est, 1.0)));
        Stat s;
        std::vector<double> v;
        for (int rep = 0; rep < REPS; ++rep)
        {
            auto a = clk::now();
            for (long k = 0; k < calls; ++k) sink ^= c.H(buf)[k % 32];
            double ns = std::chrono::duration<double, std::nano>(clk::now() - a).count() / calls;
            s.add(ns);
            v.push_back(ns);
            fprintf(raw, "%zu,%zu,%d,%ld,%.2f\n", ln, buf.size(), rep, calls, ns);
        }
        double var = 0;
        for (double x : v) var += (x - s.avg()) * (x - s.avg());
        double sd = std::sqrt(var / (v.size() - 1));
        fprintf(sum, "%zu,%zu,%.2f,%.2f,%.2f,%.2f,%.4f\n", ln, buf.size(), s.avg(), s.mn, s.mx, sd, s.avg() / buf.size());
    }
    fclose(raw);
    fclose(sum);
    printf("Sparta baigta (sink=%d)\n", (int)sink);
}

// Pasukimų analizė (H4, H5): rotate priklauso tik nuo n ir j.
void expRotations(Ctx& c)
{
    const int ROT[8] = {5, 9, 13, 17, 21, 25, 29, 3};
    FILE* f = c.open("rotations.csv");
    fprintf(f, "j,n_mod_32_when_rotate_mod32_is_0,first_n_int_overflow,int_value_at_2^28+1\n");
    for (int j = 0; j < 8; ++j)
    {
        std::string zeros;
        for (int m = 0; m < 32; ++m)
            if ((((uint64_t)(j + 1) * m) - j + ROT[j]) % 32 == 0) zeros += std::to_string(m) + " ";
        // pirmas n, kai tikroji reikšmė > INT_MAX (v0.1 naudoja int)
        uint64_t n0 = (2147483647ull + j - ROT[j]) / (j + 1) + 1;
        uint64_t nn = (1ull << 28) + 1;
        int wrapped = (int)((uint64_t)(j + 1) * nn - j + ROT[j]);
        fprintf(f, "%d,\"%s\",%llu,%d\n", j, zeros.c_str(), (unsigned long long)n0, wrapped);
    }
    fclose(f);
}
