#pragma once

namespace Lumina::Essence {

#ifndef NDEBUG
    #define LUMINA_DEBUG
#else
    #define LUMINA_RELEASE
#endif

enum class BuildMode {
    Debug,
    Release,

#if defined(LUMINA_DEBUG)
    Current = Debug,
#elif defined(LUMINA_RELEASE)
    Current = Release,
#else
    #error "Lumina is trying to compile in neither release nor debug mode which is impossible"
#endif
};

#if defined(__clang__)
    #define LUMINA_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
    #define LUMINA_COMPILER_GCC
#elif defined(_MSC_VER)
    #define LUMINA_COMPILER_MSVC
#else
    #error "Unknown compiler."
#endif
}