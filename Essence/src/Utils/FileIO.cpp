#include "Lumina/Essence/Utils/FileIO.hpp"

#include <fstream>
#include <format>

namespace Lumina::Essence {

std::vector<char> readBinaryFile(std::string const& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file \"{}\"!", path));
    }

    size_t fileSize = file.tellg();
    std::vector<char> bytes(fileSize);
    file.seekg(0);
    file.read(bytes.data(), bytes.size());
    file.close();

    return bytes;
}

}