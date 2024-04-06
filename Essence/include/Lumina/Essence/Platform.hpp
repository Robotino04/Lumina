#pragma once

namespace Lumina::Essence {

#ifndef NDEBUG
    #define LUMINA_DEBUG
#else
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
    #error Lumina is trying to compile in neither release nor debug mode which is impossible
#endif
};

}