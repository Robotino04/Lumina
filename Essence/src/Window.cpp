#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/Utils/unimplemented.hpp"

#include <iostream>

namespace Lumina::Essence {

Window::Window(glm::ivec2 size, std::string const& title) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(title.c_str(), size.x, size.y, 0);
}

Window::~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::optional<SDL_Event> Window::GetEvent() {
    SDL_Event e;
    return SDL_PollEvent(&e) != 0 ? e : std::optional<SDL_Event>{};
}


// vk::SurfaceKHR Window::CreateWindowSurface(vk::Instance instance) const {
//     unimplemented();
// }

}