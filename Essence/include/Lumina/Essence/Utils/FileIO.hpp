#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace Lumina::Essence {

std::vector<uint8_t> readBinaryFile(std::string const& path);

}