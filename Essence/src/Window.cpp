#include "Lumina/Essence/Window.hpp"

#include <iostream>

namespace Lumina::Essence {

Window::Window(glm::ivec2 size, std::string const& title) {
    {
        auto lock = std::lock_guard(glfwInitLock);
        if (glfwReferenceCount == 0) {
            if (glfwInit() == GLFW_FALSE) {
                throw std::runtime_error("GLFW initialization failed.\n");
            }
        }
        glfwReferenceCount++;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwHandle = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(glfwHandle);

    {
        auto lock = std::lock_guard(glfwInitLock);
        glfwReferenceCount--;
        if (glfwReferenceCount == 1) {
            glfwTerminate();
        }
    }
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(glfwHandle);
}


void Window::PollEvents() {
    glfwPollEvents();
}

}