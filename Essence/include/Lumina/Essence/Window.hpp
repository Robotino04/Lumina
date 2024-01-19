#pragma once

#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/GLFWReference.hpp"
#include "Lumina/Essence/Utils/NonCopyable.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <mutex>
#include <atomic>

namespace Lumina::Essence {

class Window : NonCopyable {
public:
    Window(glm::ivec2 size, std::string const& title);
    ~Window();

    bool ShouldClose() const;

    static void PollEvents();

    vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) const;
    glm::ivec2 GetFramebufferSize() const;

private:
    GLFWwindow* glfwHandle = nullptr;

    GLFWReference glfwKeepInitialized;
};

}