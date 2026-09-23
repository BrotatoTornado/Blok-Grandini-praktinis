#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using Hasher = std::array<uint8_t, 32>;

Hasher hashingBytes(const std::vector<uint8_t>& data);

Hasher hashingText(const std::string& utf8_text);

std::string ToHex(const Hasher& d);

bool isItUTF(const std::string& s);