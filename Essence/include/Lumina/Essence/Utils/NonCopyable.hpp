#pragma once

namespace Lumina::Essence {

class NonCopyable {
    public:
        NonCopyable(){};
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
};

}