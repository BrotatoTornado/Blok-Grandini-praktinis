// Regresija: maišos turi likti bit-for-bit tokios pačios. test_golden <failas> [algo]
#include "../versions/algo.h"
#include <fstream>
#include <iostream>

static std::vector<uint8_t> unhex(const std::string& s)
{
    std::vector<uint8_t> v;
    if (s == "-") return v;
    for (size_t i = 0; i + 1 < s.size(); i += 2) v.push_back(std::stoi(s.substr(i, 2), nullptr, 16));
    return v;
}

int main(int argc, char** argv)
{
    std::ifstream f(argc > 1 ? argv[1] : "tests/golden_v0_1.txt");
    std::string in, want;
    HashFn H = getAlgo(argc > 2 ? argv[2] : "v0.1");
    int n = 0, bad = 0;
    while (f >> in >> want)
    {
        ++n;
        if (toHex(H(unhex(in))) != want) { ++bad; std::cerr << "NESUTAMPA: " << in << "\n"; }
    }
    std::cout << n << " tikrinta, " << bad << " klaidu\n";
    return (n == 0 || bad) ? 1 : 0;
}
