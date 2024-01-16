#include "Lumina/Essence/Application.hpp"

#include <format>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Lumina::Essence {

Application::Application(std::string const& appName): Name(appName) {}
Application::~Application() {
    instance.destroy();
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";

    vk::ApplicationInfo appInfo(
        Name.c_str(), vk::makeVersion(1, 0, 0), "Lumina", vk::makeVersion(1, 0, 0), vk::makeApiVersion(0, 1, 0, 0)
    );

    const auto pprintVersion = [](uint32_t v) {
        return "v" + std::to_string(vk::versionMajor(v)) + "." + std::to_string(vk::versionMinor(v)) + "."
             + std::to_string(vk::versionPatch(v));
    };

    std::cout << "Initializing Vulkan: \n - " << appInfo.pApplicationName << " "
              << pprintVersion(appInfo.applicationVersion) << "\n - Lumina " << pprintVersion(appInfo.engineVersion)
              << "\n - Vulkan API " << pprintVersion(appInfo.apiVersion) << "\n";

    std::cout << "Available Vulkan extensions: \n";
    auto availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (auto ext : availableExtensions) {
        std::cout << " - " << ext.extensionName << " (" << pprintVersion(ext.specVersion) << ")\n";
    }

    std::cout << "Extensions to load:\n";
    auto requiredExtensions = GetRequiredVulkanExtensions();
    for (auto ext : requiredExtensions) {
        std::cout << " - " << ext << "\n";
    }

    vk::InstanceCreateInfo instanceInfo{
        {},
        &appInfo,
        {},
        requiredExtensions,
    };

    vk::Instance instance = vk::createInstance(instanceInfo);

    IsInitialized = true;
}


void Application::Run() {
    if (!IsInitialized)
        throw std::logic_error(
            "Tried running application without initializing it first. Maybe call "
            "the parents Initialize functions in derived classes."
        );

    IsRunning = true;
    while (IsRunning) {
        Tick(1 / 60.0f);
    }
}
void Application::Exit() {
    IsRunning = false;
}


void Application::Tick(float dt) {}

std::vector<const char*> Application::GetRequiredVulkanExtensions() {
    return {};
}
}