#pragma once

#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/Utils/NonCopyable.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <string>

namespace Lumina::Essence {

class Window : NonCopyable {
public:
    Window(glm::ivec2 size, std::string const& title);
    ~Window();

    bool ShouldClose() const;

    void PollEvents();

    // vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) const;

private:
    struct SDL_Window* window;
};

}