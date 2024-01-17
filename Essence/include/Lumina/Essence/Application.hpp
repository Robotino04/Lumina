#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/GLFWReference.hpp"
#include "Lumina/Essence/Vulkan.hpp"

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
    virtual std::vector<const char*> GetRequiredVulkanExtensions() const;
    virtual std::vector<const char*> GetRequiredVulkanValidationLayers() const;
    virtual int ScoreDeviceSuitability(vk::PhysicalDevice dev) const;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanValidationlayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData
    );

    void CreateVulkanInstance();
    void PickPhysicalDevice();

    bool IsRunning = false;
    bool IsInitialized = false;

    vk::Instance instance;
    std::optional<vk::DebugUtilsMessengerEXT> debugMessenger;
    vk::PhysicalDevice physicalDevice;
};

}