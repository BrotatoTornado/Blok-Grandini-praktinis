#include "muhhash.h"

inline const uint32_t IV[8] = {
    0x1A2B3C4Du, 0x5E6F7081u, 0x92A3B4C5u, 0xD6E7F809u,
    0x1B2C3D4Eu, 0x5F607182u, 0x93A4B5C6u, 0xD7E8F90Au,
};
inline const int ROT[8] = {5, 9, 13, 17, 21, 25, 29, 3};
inline const uint32_t MUL = 0x9E3779B1u;
inline const int ROUNDS = 4;

uint32_t rotl(uint32_t x, uint32_t r)
{
    r &= 31;
    if (r == 0)
    {
        return x;
    }
    return (x << r) | (x >> (32 - r));
}

Hasher hashBytes(const std::vector<uint8_t>& data)
{
    uint64_t n = data.size();
    uint32_t s[8];
    for (int j = 0; j < 8; ++j)
    {
        s[j] = IV[j];
    }

    for (uint64_t i = 0; i < n; ++i)
    {
        int j = i % 8;
        s[j] += data[i];
        s[j] = rotl(s[j], (j + 1) * n - j + ROT[j]);
        s[j] ^= s[(j + 3) % 8];
        s[j] *= MUL;
    }

    // 64 bitai į dvi juostas.
    s[0] += static_cast<uint32_t>(n);
    s[1] += static_cast<uint32_t>(n >> 32);

    for (int r = 0; r < ROUNDS; ++r)
    {
        for (int j = 0; j < 8; ++j)
        {
            s[j] += s[(j + 1) % 8];
            s[j] = rotl(s[j], ROT[j]);
            s[j] ^= s[(j + 5) % 8];
            s[j] *= MUL;
        }
    }

    Hasher out;
    for (int j = 0; j < 8; ++j)
    {
        for (int b = 0; b < 4; ++b)
        {
            out[j * 4 + b] = s[j] >> (24 - 8 * b);
        }
    }
    return out;
}

std::string toHex(const Hasher& h)
{
    const char* digits = "0123456789abcdef";
    std::string s;
    for (uint8_t b : h)
    {
        s += digits[b >> 4];
        s += digits[b & 0xF];
    }
    return s;
}

// ne įprastą failą laiko klaida.
std::vector<uint8_t> readFile(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
    {
        throw std::runtime_error("Nera iprasto failo: " + path);
    }
    std::ifstream f(path, std::ios::binary);
    std::vector<uint8_t> data(std::filesystem::file_size(path));
    f.read(reinterpret_cast<char*>(data.data()), data.size());
    if (!f)
    {
        throw std::runtime_error("Klaida skaitant faila: " + path);
    }
    return data;
}


bool isUtf8(const std::string& s)
{
    size_t i = 0;
    while (i < s.size())
    {
        uint8_t b = s[i];
        int extra = 0;
        uint32_t code = b;
        uint32_t minCode = 0;

        if (b >= 0xF0 && b < 0xF8)
        {
            extra = 3;
            code = b & 0x07;
            minCode = 0x10000;
        }
        else if (b >= 0xE0 && b < 0xF0)
        {
            extra = 2;
            code = b & 0x0F;
            minCode = 0x800;
        }
        else if (b >= 0xC0 && b < 0xE0)
        {
            extra = 1;
            code = b & 0x1F;
            minCode = 0x80;
        }
        else if (b >= 0x80)
        {
            return false;
        }

        if (i + extra >= s.size())
        {
            return false;
        }
        for (int k = 1; k <= extra; ++k)
        {
            uint8_t c = s[i + k];
            if ((c & 0xC0) != 0x80)
            {
                return false;
            }
            code = (code << 6) | (c & 0x3F);
        }
        if (code < minCode || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        {
            return false;
        }
        i += extra + 1;
    }
    return true;
}
