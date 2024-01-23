#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/Window.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace Lumina::Essence {

class Application : NonCopyable {
public:
    Application(glm::ivec2 windowSize, std::string const& windowTitle);
    virtual ~Application();
    virtual void Initialize();

    virtual void Tick(float dt);

    virtual void PreRender(float dt);
    virtual void Render(float dt);
    virtual void PostRender(float dt);

    void Run();
    void Exit();

    const std::string Name;

protected:
    Window window;
    uint32_t currentImageIndex;

private:
    bool IsRunning = false;
    bool IsInitialized = false;
};

}