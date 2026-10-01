// Eksperimentas 7: PIN (0000-9999) spėjimas – be druskos, struktūrinė ataka, druska, slaptas r.
#include "common.h"
#include <map>

using Clk = std::chrono::steady_clock;
static double msSince(Clk::time_point t) { return std::chrono::duration<double, std::milli>(Clk::now() - t).count(); }

static std::string pinStr(int v)
{
    char b[8];
    snprintf(b, sizeof b, "%04d", v);
    return b;
}
static Bytes withSuffix(const std::string& s, const Bytes& suf)
{
    Bytes b = toBytes(s);
    b.insert(b.end(), suf.begin(), suf.end());
    return b;
}
static Bytes randBytes(Rng& r, size_t n)
{
    Bytes b(n);
    for (auto& x : b) x = uint8_t(r.below(256));
    return b;
}

// Struktūrinė ataka (H3): kiekvienos juostos porą lygina su "dddd" maiša.
// Gauna tik maišą; suf – viešas priedas (druska), gali būti tuščias.
static bool structural(Ctx& c, const Hasher& target, const Bytes& suf, const Hasher hd[10], int& found)
{
    int dig[4] = {-1, -1, -1, -1};
    for (int p = 0; p < 4; ++p)
        for (int d = 0; d < 10 && dig[p] < 0; ++d)
            if (lane(hd[d], p) == lane(target, p)) dig[p] = d;
    for (int p = 0; p < 4; ++p)
        if (dig[p] < 0) return false;
    found = dig[0] * 1000 + dig[1] * 100 + dig[2] * 10 + dig[3];
    return c.H(withSuffix(pinStr(found), suf)) == target; // patikra pilna maiša
}

void expGuess(Ctx& c)
{
    Rng r(SEED + 7);
    FILE* f = c.open("guess.csv");
    fprintf(f, "section,metric,value\n");
    int target = int(r.below(10000));
    Hasher th = c.H(toBytes(pinStr(target)));
    fprintf(f, "setup,target_pin,%s\nsetup,target_hash,%s\n", pinStr(target).c_str(), toHex(th).c_str());

    // 1. Be druskos
    {
        auto t = Clk::now();
        std::vector<int> matches;
        int first = 0;
        std::map<std::string, int> distinct;
        for (int i = 0; i < 10000; ++i)
        {
            Hasher h = c.H(toBytes(pinStr(i)));
            ++distinct[toHex(h)];
            if (h == th)
            {
                matches.push_back(i);
                if (!first) first = i + 1;
            }
        }
        fprintf(f, "no_salt,attempts_to_first_match,%d\nno_salt,attempts_full_scan,10000\n", first);
        fprintf(f, "no_salt,time_ms_full_scan,%.3f\nno_salt,matching_candidates,%zu\n", msSince(t), matches.size());
        std::string lst;
        for (int m : matches) lst += pinStr(m) + " ";
        fprintf(f, "no_salt,matching_candidate_list,%s\n", lst.c_str());
        fprintf(f, "no_salt,distinct_hashes_among_10000,%zu\n", distinct.size());
    }
    // 2. Struktūrinė ataka prieš visus 10000 taikinių
    {
        auto t = Clk::now();
        Hasher hd[10];
        for (int d = 0; d < 10; ++d) hd[d] = c.H(toBytes(std::string(4, char('0' + d))));
        int ok = 0;
        for (int i = 0; i < 10000; ++i)
        {
            int got = -1;
            ok += structural(c, c.H(toBytes(pinStr(i))), {}, hd, got) && got == i;
        }
        fprintf(f, "structural,targets,10000\nstructural,successes,%d\n", ok);
        fprintf(f, "structural,hashes_precomputed,10\nstructural,hashes_verification_per_target,1\n");
        fprintf(f, "structural,brute_force_avg_attempts,5000.5\nstructural,time_ms_all_targets,%.3f\n", msSince(t));
    }
    // 3. Vieša druska (16 baitų, žali baitai; H(pin || salt))
    {
        const int T = 100;
        auto t0 = Clk::now();
        std::map<Hasher, int> table; // iš anksto skaičiuota lentelė be druskos
        for (int i = 0; i < 10000; ++i) table[c.H(toBytes(pinStr(i)))] = i;
        double tableMs = msSince(t0);
        std::vector<int> pins(T);
        std::vector<Bytes> salts(T);
        std::vector<Hasher> hs(T);
        for (int k = 0; k < T; ++k)
        {
            pins[k] = int(r.below(10000));
            salts[k] = randBytes(r, 16);
            hs[k] = c.H(withSuffix(pinStr(pins[k]), salts[k]));
        }
        int tableHits = 0;
        for (int k = 0; k < T; ++k) tableHits += table.count(hs[k]) && table[hs[k]] == pins[k];
        auto t1 = Clk::now();
        int cracked = 0;
        for (int k = 0; k < T; ++k)
            for (int i = 0; i < 10000; ++i)
                if (c.H(withSuffix(pinStr(i), salts[k])) == hs[k]) { cracked += i == pins[k]; break; }
        double perTargetMs = msSince(t1);
        // ta pati druska visiems taikiniams: viena nauja lentelė tinka visiems
        Bytes shared = randBytes(r, 16);
        auto t2 = Clk::now();
        std::map<Hasher, int> tabS;
        for (int i = 0; i < 10000; ++i) tabS[c.H(withSuffix(pinStr(i), shared))] = i;
        int sharedHits = 0;
        for (int k = 0; k < T; ++k)
        {
            auto it = tabS.find(c.H(withSuffix(pinStr(pins[k]), shared)));
            sharedHits += it != tabS.end() && it->second == pins[k];
        }
        double sharedMs = msSince(t2);
        // struktūrinė ataka su žinoma druska
        int sOk = 0, repeated = 0;
        for (int k = 0; k < T; ++k)
        {
            Hasher hd[10];
            for (int d = 0; d < 10; ++d) hd[d] = c.H(withSuffix(std::string(4, char('0' + d)), salts[k]));
            int got = -1;
            sOk += structural(c, hs[k], salts[k], hd, got) && got == pins[k];
            repeated += pins[k] % 1111 == 0;
        }
        fprintf(f, "salt,targets,%d\nsalt,precomputed_table_ms_10000_hashes,%.3f\n", T, tableMs);
        fprintf(f, "salt,unsalted_table_hits_on_salted_targets,%d\n", tableHits);
        fprintf(f, "salt,distinct_salts_hashes_needed,%d\nsalt,distinct_salts_cracked,%d\n", T * 10000, cracked);
        fprintf(f, "salt,distinct_salts_time_ms_worst_case_scan,%.3f\n", perTargetMs);
        fprintf(f, "salt,same_salt_hashes_needed,10000\nsalt,same_salt_cracked,%d\n", sharedHits);
        fprintf(f, "salt,same_salt_time_ms,%.3f\nsalt,structural_attack_successes_with_known_salt,%d\nsalt,targets_with_four_equal_digits,%d\n", sharedMs, sOk, repeated);
    }
    // 4. Slaptas r: paieškos erdvė 10^4 * 2^128; parodoma tik commit/reveal patikra
    {
        Bytes rr = randBytes(r, 16);
        Hasher com = c.H(withSuffix(pinStr(target), rr));
        Bytes rr2 = rr;
        rr2[0] ^= 1;
        fprintf(f, "secret_r,search_space,10^4 * 2^128 = 3.4e42\n");
        fprintf(f, "secret_r,reveal_correct_ok,%d\n", c.H(withSuffix(pinStr(target), rr)) == com);
        fprintf(f, "secret_r,reveal_wrong_pin_ok,%d\n", c.H(withSuffix(pinStr((target + 1) % 10000), rr)) == com);
        fprintf(f, "secret_r,reveal_wrong_r_ok,%d\n", c.H(withSuffix(pinStr(target), rr2)) == com);
    }
    fclose(f);
}
