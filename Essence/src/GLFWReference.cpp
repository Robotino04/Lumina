#include "Lumina/Essence/GLFWReference.hpp"

#include <iostream>
#include <stdexcept>

namespace Lumina::Essence {

GLFWReference::GLFWReference() {
    auto lock = std::lock_guard(glfwInitLock);
    if (glfwReferenceCount == 0) {
        if (glfwInit() == GLFW_FALSE) {
            throw std::runtime_error("GLFW initialization failed.\n");
        }
    }
    glfwReferenceCount++;
}

GLFWReference::~GLFWReference() {
    auto lock = std::lock_guard(glfwInitLock);
    glfwReferenceCount--;
    if (glfwReferenceCount == 1) {
        glfwTerminate();
    }
}

}