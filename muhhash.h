#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using Hasher = std::array<uint8_t, 32>;

uint32_t rotl(uint32_t x, uint32_t r);

Hasher hashBytes(const std::vector<uint8_t>& data);

std::string toHex(const Hasher& h);

std::vector<uint8_t> readFile(const std::string& path);

bool isUtf8(const std::string& s);
