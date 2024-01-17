#include "Lumina/Essence/WindowApplication.hpp"

namespace Lumina::Essence {


WindowApplication::WindowApplication(glm::ivec2 windowSize, std::string const& windowTitle)
    : window(windowSize, windowTitle), Application(windowTitle) {}

void WindowApplication::Tick(float dt) {
    Application::Tick(dt);
    window.PollEvents();

    if (window.ShouldClose()) {
        Exit();
    }
}


std::vector<const char*> WindowApplication::GetRequiredVulkanExtensions() const {
    auto parentExtensions = Application::GetRequiredVulkanExtensions();

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (int i = 0; i < glfwExtensionCount; i++) {
        parentExtensions.push_back(glfwExtensions[i]);
    }

    return parentExtensions;
}

}