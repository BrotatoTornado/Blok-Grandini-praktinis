// Eksperimentų paleidėjas: experiments <komanda> [--algo v0.1] [--data data] [--out results]
#include "common.h"
#include <cstring>

int main(int argc, char** argv)
{
    Ctx c;
    std::string cmd;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--algo" && i + 1 < argc) c.algo = argv[++i];
        else if (a == "--data" && i + 1 < argc) c.dataDir = argv[++i];
        else if (a == "--out" && i + 1 < argc) c.outDir = argv[++i];
        else if (a == "--konst" && i + 1 < argc) c.konst = argv[++i];
        else cmd = a;
    }
    c.H = getAlgo(c.algo);
    if (!c.H || cmd.empty())
    {
        fprintf(stderr, "Naudojimas: experiments <inputs|format|determinism|speed|collisions|avalanche|guess|rotations> [--algo v0.1]\n");
        return 2;
    }
    if (cmd == "inputs") expInputs(c);
    else if (cmd == "format") expFormat(c);
    else if (cmd == "determinism") expDeterminism(c);
    else if (cmd == "speed") expSpeed(c);
    else if (cmd == "collisions") expCollisions(c);
    else if (cmd == "avalanche") expAvalanche(c);
    else if (cmd == "guess") expGuess(c);
    else if (cmd == "rotations") expRotations(c);
    else { fprintf(stderr, "Nezinoma komanda\n"); return 2; }
    return 0;
}
