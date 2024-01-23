#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/Utils/unimplemented.hpp"

#include <iostream>

namespace Lumina::Essence {

Window::Window(glm::ivec2 size, std::string const& title) {
    window = SDL_CreateWindow(title.c_str(), size.x, size.y, 0);
}

Window::~Window() {
    SDL_DestroyWindow(window);
}

bool Window::ShouldClose() const {
    return false;
}

void Window::PollEvents() {
    unimplemented();
}

// vk::SurfaceKHR Window::CreateWindowSurface(vk::Instance instance) const {
//     unimplemented();
// }

}