#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

#include <iostream>

namespace Lumina::Essence {

Application::Application(glm::ivec2 windowSize, std::string const& windowTitle)
    : Name(windowTitle), window(windowSize, windowTitle) {}
Application::~Application() {
    std::cout << "Application shutting down...\n";
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";

    IsInitialized = true;
}

// VULKAN_HPP_DEFAULT_DISPATCHER.init();

void Application::Run() {
    if (!IsInitialized)
        throw std::logic_error(
            "Tried running application without initializing it first. Maybe call the parents Initialize() "
            "functions in derived classes."
        );

    IsRunning = true;
    float dt = 1.0f / 60.0f;
    while (IsRunning) {
        Tick(dt);

        PreRender(dt);
        Render(dt);
        PostRender(dt);
    }
}
void Application::Exit() {
    IsRunning = false;
}


void Application::Tick(float dt) {
    window.PollEvents();

    if (window.ShouldClose()) {
        Exit();
    }
}
void Application::PreRender(float dt) {}
void Application::Render(float dt) {}
void Application::PostRender(float dt) {}

}