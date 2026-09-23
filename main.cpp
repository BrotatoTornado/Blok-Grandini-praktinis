#include <cstdio>
#include <iostream>
#include <string>

#include "muhhash.h"

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
    return tekstas();
}