#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/Vulkan.hpp"

#include <vector>
#include <string>
#include <optional>

namespace Lumina::Essence {

class Application : NonCopyable {
public:
    Application(glm::ivec2 windowSize, std::string const& windowTitle);
    virtual ~Application();
    virtual void Initialize();

    virtual void Tick(float dt);

    void Run();
    void Exit();


    const std::string Name;

protected:
    vk::Instance instance;
    std::optional<vk::DebugUtilsMessengerEXT> debugMessenger;
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::Queue graphicsQueue;
    std::optional<vk::Queue> presentQueue;

    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
    std::vector<vk::Framebuffer> swapchainFramebuffers;

    vk::Format swapchainImageFormat;
    vk::Extent2D swapchainExtent;

    vk::PipelineLayout pipelineLayout;
    vk::RenderPass renderPass;
    vk::Pipeline graphicsPipeline;

    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    Window window;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanValidationlayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData
    );

    void CreateVulkanInstance();
    void CreateVulkanSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffer();

    void RecordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    vk::ShaderModule CreateShaderModule(std::vector<char> const& bytecode) const;

    std::vector<const char*> GetRequiredVulkanExtensions() const;
    std::vector<const char*> GetRequiredVulkanValidationLayers() const;
    std::vector<const char*> GetRequiredVulkanDeviceExtensions() const;
    int ScoreDeviceSuitability(vk::PhysicalDevice dev) const;


    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    QueueFamilyIndices GetQueueFamilyIndices(vk::PhysicalDevice dev) const;
    bool DeviceSupportsAllExtensions(vk::PhysicalDevice dev) const;


    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    SwapChainSupportDetails QuerySwapchainSupport(vk::PhysicalDevice device) const;

    vk::SurfaceFormatKHR ChooseSwapchainSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats) const;
    vk::PresentModeKHR ChooseSwapchainPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) const;
    vk::Extent2D ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR const& capabilities) const;

    bool IsRunning = false;
    bool IsInitialized = false;
};

}