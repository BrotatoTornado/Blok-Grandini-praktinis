// Naudojimas: muhhash [-q] [--text | <failas>]   (be argumentų – meniu)
#include <cstring>
#include <iostream>

#include "muhhash.h"

static bool quiet = false; // -q: informacija į stderr, stdout tik maiša

static void info(const std::string& msg)
{
    (quiet ? std::cerr : std::cout) << msg;
}

static int hashData(const std::vector<uint8_t>& data)
{
    info("Baitu: " + std::to_string(data.size()) + "\n");
    std::cout << toHex(hashBytes(data)) << "\n";
    return 0;
}

static int hashFile(const std::string& path)
{
    try
    {
        return hashData(readFile(path));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Blogai: " << e.what() << "\n";
        return 1;
    }
}

static int hashText()
{
    info("Ivesk teksta ir paspausk Enter:\n");
    std::string line;
    std::getline(std::cin, line);
    if (!isUtf8(line))
    {
        std::cerr << "Blogai: ne UTF-8 ivestis\n";
        return 1;
    }
    return hashData(std::vector<uint8_t>(line.begin(), line.end()));
}

int main(int argc, char* argv[])
{
    bool text = false;
    std::string path;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-q"))
        {
            quiet = true;
        }
        else if (!strcmp(argv[i], "--text"))
        {
            text = true;
        }
        else
        {
            path = argv[i];
        }
    }

    if (text)
    {
        return hashText();
    }
    if (!path.empty())
    {
        return hashFile(path);
    }

    while (true)
    {
        std::cout << "Ivestis ranka ar failas?\n1 - Ranka\n2 - Failas\n";
        int choice = 0;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 1)
        {
            return hashText();
        }
        if (choice == 2)
        {
            std::cout << "Failo kelias: ";
            std::getline(std::cin, path);
            return hashFile(path);
        }
        if (std::cin.eof())
        {
            return 1;
        }
        std::cout << "Blogas pasirinkimas, bandyk dar karta.\n\n";
        std::cin.clear();
        std::cin.ignore(1000, '\n');
    }
}
