#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/GLFWReference.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <string>

namespace Lumina::Essence {

class Application : NonCopyable {
public:
    Application(std::string const& appName);
    virtual ~Application();
    virtual void Initialize();

    virtual void Tick(float dt);

    void Run();
    void Exit();


    const std::string Name;

protected:
    virtual std::vector<const char*> GetRequiredVulkanExtensions();

private:
    bool IsRunning = false;
    vk::Instance instance;
    bool IsInitialized = false;
};

}