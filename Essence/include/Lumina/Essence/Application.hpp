#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/GLFWReference.hpp"
#include "Lumina/Essence/Vulkan.hpp"

#include <vector>
#include <string>
#include <optional>

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
    virtual std::vector<const char*> GetRequiredVulkanPerDeviceExtensions() const;
    virtual int ScoreDeviceSuitability(vk::PhysicalDevice dev) const;

    virtual std::optional<vk::SurfaceKHR> CreateVulkanSurface() const;

    vk::Instance instance;
    std::optional<vk::DebugUtilsMessengerEXT> debugMessenger;
    std::optional<vk::SurfaceKHR> surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    std::optional<vk::Queue> presentQueue;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanValidationlayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData
    );

    void CreateVulkanInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    bool IsRunning = false;
    bool IsInitialized = false;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete(bool needsPresentation) const {
            return graphicsFamily.has_value() && (presentFamily.has_value() || !needsPresentation);
        }
    };
    QueueFamilyIndices GetQueueFamilyIndices(vk::PhysicalDevice dev) const;
};

}