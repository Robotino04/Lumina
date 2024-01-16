#pragma once

namespace Lumina::Essence {

#ifndef NDEBUG
    #define LUMINA_DEBUG
#elif
    #define LUMINA_RELEASE
#endif

enum BuildMode {
    Debug,
    Release,

#if defined(LUMINA_DEBUG)
    Current = Debug,
#elif defined(LUMINA_RELEASE)
    Current = Release,
#else
    #error Lumina is trying to compile in both release and debug mode which is impossible
#endif
};

}