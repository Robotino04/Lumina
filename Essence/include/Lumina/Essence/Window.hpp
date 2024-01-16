#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <mutex>
#include <atomic>

#include "Lumina/Essence/Utils/NonCopyable.hpp"

namespace Lumina::Essence {

class Window : NonCopyable {
public:
    Window(glm::ivec2 size, std::string const& title);
    ~Window();

    bool ShouldClose() const;

    static void PollEvents();

private:
    GLFWwindow* glfwHandle = nullptr;

    static inline std::mutex glfwInitLock;

    /**
     * @brief How many windows still need glfw to be initialized.
     *
     */
    static inline std::atomic<uint> glfwReferenceCount = 0;
};

}