#pragma once

#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Window.hpp"

namespace Lumina::Essence {


class WindowApplication : public Application {
public:
    WindowApplication(glm::ivec2 windowSize, std::string const& windowTitle);

    void Tick(float dt) override;

protected:
    std::vector<const char*> GetRequiredVulkanExtensions() const override;

    Window window;
};

}