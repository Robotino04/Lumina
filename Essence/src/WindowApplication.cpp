#include "Lumina/Essence/WindowApplication.hpp"

namespace Lumina::Essence {


WindowApplication::WindowApplication(glm::ivec2 windowSize, std::string const& windowTitle)
    : window(windowSize, windowTitle) {}

void WindowApplication::Tick(float dt) {
    Application::Tick(dt);
    window.PollEvents();

    if (window.ShouldClose()) {
        Exit();
    }
}

}