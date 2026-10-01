// Regresijos testas: maišos turi likti tokios pačios (v0.2 reikšmės) + UTF-8 patikra.
#include <iostream>

#include "muhhash.h"

struct Case { const char* input; const char* hash; };

static const Case cases[] = {
    {"", "b04f9d7088734b417521fa65f36e245a5985adbcf5152ab9bbc9708e7c499716"},
    {"\x61", "ae91a3bb374f40cdcd3e027b1956050dd49b5efd5f9e9da6b4a88609e754b882"},
    {"\x62", "7908849aa88019d692476746df168323192fef7d3faab726dfdbdd30a333ab49"},
    {"\x61\x62\x63", "b0c016e09387471998e6796e8a1519f8cdcfc2eac8723f83a104b96a9964bd7e"},
    {"\x61\x63\x62", "e32039305656dbe32955c606610aa983ea1106213703e61009e7e0e55c60a81a"},
    {"\x62\x63\x61", "171ffe52a535d8d1042f87437434a9ed5ccc6ce12a12d7db24caa47e5a055295"},
    {"\x4c\x69\x65\x74\x75\x76\x61", "05feb4c268c4ee6ceba6356a72b2d5021ebc5d5b512d5503fd2d4d260193753f"},
    {"\x4c\x69\x65\x74\x75\x76\x61\x0a", "0245bbc47a50abc5ab8e781a1e3074754b61a3d846dfce99a094e5b1ce75a05e"},
    {"\x4c\x69\x65\x74\x75\x76\x61\x21", "f22ae7a9c32d4d4abd7b751a43f7aa4ac90913c317bf3e1374242eb0f34fbc0c"},
    {"\x6c\x69\x65\x74\x75\x76\x61", "ab79bd843250d3f3710c1f709f5a2ce1715760a3c5b403d6e53f210a3913a3b1"},
    {"\xc4\x85\xc4\x8d\xc4\x99\xc4\x97\xc4\xaf\xc5\xa1\xc5\xb3\xc5\xab\xc5\xbe", "c05ef282e6e52476a3e20f693f1ad147a9498dad2aa30bd16a7f713e2ff5a4be"},
    {"\x20\x78\x20", "2236f9d1abadbcab249fc541bc06b5b7d809cdfecbc25320e6e51c16b66c0104"},
};

int main()
{
    int bad = 0;
    for (const Case& c : cases)
    {
        std::string s = c.input;
        if (toHex(hashBytes(std::vector<uint8_t>(s.begin(), s.end()))) != c.hash)
        {
            std::cerr << "NESUTAMPA: " << s << "\n";
            ++bad;
        }
    }

    // Klaidinga UTF-8: nutrūkusi seka, per ilgas kodas, surogatas.
    const char* badUtf[] = {"\xC4", "\xC0\x80", "\xED\xA0\x80"};
    for (const char* s : badUtf)
    {
        if (isUtf8(s))
        {
            std::cerr << "UTF-8 turejo buti klaida\n";
            ++bad;
        }
    }
    if (!isUtf8("ąčęėįšųūž") || !isUtf8(""))
    {
        std::cerr << "UTF-8 turejo buti teisingas\n";
        ++bad;
    }

    std::cout << bad << " klaidu\n";
    return bad != 0;
}
