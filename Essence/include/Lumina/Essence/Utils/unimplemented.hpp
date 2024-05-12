#pragma once

#include <source_location>
#include <stdexcept>
#include <format>

namespace Lumina::Essence {

[[noreturn]] inline void Unimplemented(std::source_location loc = std::source_location::current()) {
    throw std::runtime_error(
        std::format("{}:{}:{} {} is not yet implemented", loc.file_name(), loc.line(), loc.column(), loc.function_name())
    );
}

}