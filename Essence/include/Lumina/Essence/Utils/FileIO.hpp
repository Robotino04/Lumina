#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace Lumina::Essence {

std::vector<uint8_t> ReadBinaryFile(std::string const& path);

}