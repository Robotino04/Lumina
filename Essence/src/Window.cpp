#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/Utils/unimplemented.hpp"

#include <SDL3/SDL_vulkan.h>

#include <iostream>

namespace Lumina::Essence {

Window::Window(glm::ivec2 size, std::string const& title) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(title.c_str(), size.x, size.y, SDL_WINDOW_VULKAN);
    SDL_StartTextInput();
}

Window::~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::optional<SDL_Event> Window::GetEvent() {
    SDL_Event e;
    return SDL_PollEvent(&e) != 0 ? e : std::optional<SDL_Event>{};
}


vk::SurfaceKHR Window::CreateWindowSurface(vk::Instance instance) const {
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface) != SDL_TRUE) {
        throw std::runtime_error(std::format("SDL Error: {}", SDL_GetError()));
    }
    return surface;
}

}