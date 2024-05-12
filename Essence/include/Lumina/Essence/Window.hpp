#pragma once

#include "Lumina/Essence/Vulkan.hpp" // IWYU pragma: keep
#include "Lumina/Essence/Utils/NonCopyable.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <string>
#include <optional>

namespace Lumina::Essence {

class Window : NonCopyable {
public:
    Window(glm::ivec2 size, std::string const& title);
    ~Window();

    std::optional<SDL_Event> GetEvent();

    inline SDL_Window* GetRawWindow() {
        return window;
    }

    vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) const;

private:
    struct SDL_Window* window;
};

}