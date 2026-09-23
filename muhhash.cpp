#include "muhhash.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

inline uint32_t rotl(uint32_t x, int r)
{
    r % 32;
    if (r == 0)
    {
        return x;
    }
    return (x << r) | (x >> (32 - r));
}

constexpr uint32_t IV[8] =
{
    0x1A2B3C4Du, 0x5E6F7081u, 0x92A3B4C5u, 0xD6E7F809u,
    0x1B2C3D4Eu, 0x5F607182u, 0x93A4B5C6u, 0xD7E8F90Au,
};

const int ROT[8] = { 5, 9, 13, 17, 21, 25, 29, 3 };

Hasher hashingBytes(const std::vector<uint8_t>& data)
{
    uint32_t state[8];
    for (int i = 0; i < 8; ++i)
    {
        state[i] = IV[i];
    }

    for (size_t i = 0; i < data.size(); ++i)
    {
        int j = i % 8;
        state[j] += data[i];
        state[j] = rotl(state[j], ROT[j]);
        state[j] ^= state[(j + 3) % 8];
    }

    Hasher out;
    for (int i = 0; i < 8; ++i)
    {
        out[i * 4 + 0] = state[i] >> 24;
        out[i * 4 + 1] = state[i] >> 16;
        out[i * 4 + 2] = state[i] >> 8;
        out[i * 4 + 3] = state[i];
    }
    return out;
}

Hasher hashingText(const std::string& utf8_text)
{
    return hashingBytes(std::vector<uint8_t>(utf8_text.begin(), utf8_text.end()));
}

Hasher hashingFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
    {
        throw std::runtime_error("Nepavyko atidaryti failo: " + path);
    }

    std::vector<uint8_t> data;
    char ch;
    while (f.get(ch))
    {
        data.push_back(static_cast<uint8_t>(ch));
    }

    if (f.bad())
    {
        throw std::runtime_error("Klaida skaitant faila: " + path);
    }

    return hashingBytes(data);
}


std::string ToHex(const Hasher& d)
{
    static const char* hx = "0123456789abcdef";
    std::string s;
    s.reserve(32 * 2);
    for (uint8_t b : d)
    {
        s.push_back(hx[b >> 4]);
        s.push_back(hx[b & 0xF]);
    }
    return s;
}

bool isItUTF(const std::string& s)
{
    size_t i = 0;
    size_t n = s.size();

    while (i < n)
    {
        uint8_t b0 = s[i];

        if (b0 <= 0x7F)
        {
            i += 1;
            continue;
        }

        size_t extra_bytes = 0;
        uint32_t codepoint = 0;
        uint32_t min_codepoint = 0;

        if ((b0 & 0xE0) == 0xC0)
        {
            extra_bytes = 1;
            codepoint = b0 & 0x1F;
            min_codepoint = 0x80;
        }
        else if ((b0 & 0xF0) == 0xE0)
        {
            extra_bytes = 2;
            codepoint = b0 & 0x0F;
            min_codepoint = 0x800;
        }
        else if ((b0 & 0xF8) == 0xF0)
        {
            extra_bytes = 3;
            codepoint = b0 & 0x07;
            min_codepoint = 0x10000;
        }
        else
        {
            return false;
        }

        if (i + extra_bytes >= n)
        {
            return false;
        }

        for (size_t k = 1; k <= extra_bytes; ++k)
        {
            uint8_t bk = s[i + k];
            if ((bk & 0xC0) != 0x80)
            {
                return false;
            }
            codepoint = (codepoint << 6) | (bk & 0x3F);
        }

        if (codepoint < min_codepoint)
        {
            return false;
        }

        if (codepoint > 0x10FFFF)
        {
            return false;
        }

        if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
        {
            return false;
        }

        i += extra_bytes + 1;
    }

    return true;
}
