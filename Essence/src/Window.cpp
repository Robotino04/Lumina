#include "Lumina/Essence/Window.hpp"

#include <iostream>

namespace Lumina::Essence {

Window::Window(glm::ivec2 size, std::string const& title) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwHandle = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(glfwHandle);
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(glfwHandle);
}


void Window::PollEvents() {
    glfwPollEvents();
}

vk::SurfaceKHR Window::CreateWindowSurface(vk::Instance instance) const {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, glfwHandle, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    return surface;
}

}