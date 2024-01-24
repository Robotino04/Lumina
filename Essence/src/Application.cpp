#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

#include <iostream>
#include <chrono>
#include <thread>

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
        while (auto e = window.GetEvent()) {
            HandleEvent(e.value());
        }

        Tick(dt);

        if (!IsRenderingEnabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }


        PreRender(dt);
        Render(dt);
        PostRender(dt);
    }
}
void Application::Exit() {
    IsRunning = false;
}


void Application::Tick(float dt) {}
void Application::PreRender(float dt) {}
void Application::Render(float dt) {}
void Application::PostRender(float dt) {}
void Application::HandleEvent(SDL_Event e) {
    switch (e.type) {
        case SDL_EventType::SDL_EVENT_QUIT:             Exit(); break;
        case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: IsRenderingEnabled = false; break;
        case SDL_EventType::SDL_EVENT_WINDOW_MAXIMIZED: IsRenderingEnabled = true; break;
    }
}

}