#include <cstdio>
#include <iostream>
#include <string>
#include <stdexcept>

#include "muhhash.h"

int failas(const std::string& path)
{
    printf("FAILO MAISA\n");

    try
    {
        Hasher d = hashingFile(path);
        printf("%s\n", ToHex(d).c_str());
        return 0;
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Blogai: nepavyko perskaityti failo. %s\n", e.what());
        return 1;
    }
}


int tekstas()
{
    printf("Ivesk teksta ir paspausk Enter:\n");

    std::string line;
    if (!std::getline(std::cin, line))
    {
        printf("(0 baitu)\n");
        line.clear();
    }

    printf("Baitu: %zu\n\n", line.size());

    if (!isItUTF(line))
    {
        fprintf(stderr, "Blogai: ne UTF-8 ivestis\n");
        return 1;
    }

    Hasher d = hashingText(line);
    printf("%s\n", ToHex(d).c_str());
    return 0;
}

int main()
{
    while (true)
    {
        printf("Ivestis ranka ar failas?\n1 - Ranka\n2 - Failas\n");

        int c = 0;
        std::cin >> c;
        std::cin.ignore();

        if (c == 1)
        {
            return tekstas();
        }
        else if (c == 2)
        {
            printf("Failo kelias: ");
            std::string kelias;
            std::getline(std::cin, kelias);
            return failas(kelias);
        }
        else
        {
            printf("Blogas pasirinkimas, bandyk dar karta.\n\n");
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        }
    }
}
