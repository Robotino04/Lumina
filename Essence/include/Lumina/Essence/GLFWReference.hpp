#pragma once

#include <GLFW/glfw3.h>
#include <mutex>
#include <atomic>

#include "Lumina/Essence/Utils/NonCopyable.hpp"

namespace Lumina::Essence {

class GLFWReference : NonCopyable {
public:
    GLFWReference();
    ~GLFWReference();

private:
    static inline std::mutex glfwInitLock;
    static inline std::atomic<unsigned int> glfwReferenceCount = 0;

    static void glfwErrorCallback(int code, const char* description);
};

}