#include "Lumina/Essence/Utils/Platform.hpp"

#if defined(LUMINA_COMPILER_CLANG) || defined(LUMINA_COMPILER_GCC)
    #define LUMINA_PACKED(...) __VA_ARGS__ __attribute__((__packed__))
#else
    #error "LUMINA_PACKED isn't implemented for msvc"
#endif