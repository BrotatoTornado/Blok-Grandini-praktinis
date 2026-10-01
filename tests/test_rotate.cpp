// v0.2 pasukimas turi sutapti su v0.1 (int), n >= 1 (n = 0 ciklas nevyksta), n < 2^28.
#include "../versions/algo.h"
#include <cstdio>

int main()
{
    const int ROT[8] = {5, 9, 13, 17, 21, 25, 29, 3};
    long bad = 0;
    for (size_t n = 1; n < (1u << 28); n += 1)
        for (int j = 0; j < 8; ++j)
        {
            int old = (j + 1) * n - j + ROT[j]; // v0.1 formulė
            if (old % 32 != rotateAmount(n, j)) ++bad;
        }
    printf("neatitikimu: %ld\n", bad);
    return bad != 0;
}
