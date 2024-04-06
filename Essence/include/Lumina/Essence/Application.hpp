#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/VulkanImage.hpp"
#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/DeletionQueue.hpp"
#include "Lumina/Essence/DescriptorAllocator.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <array>

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

    virtual void HandleEvent(SDL_Event e);

    void Run();
    void Exit();

    const std::string name;

protected:
    struct FrameData {
        vk::CommandPool commandPool;
        vk::CommandBuffer mainCommandBuffer;

        vk::Semaphore renderSemaphore, swapchainSemaphore;
        vk::Fence renderFence;

        DeletionQueue deletionQueue;
    };

    std::array<FrameData, 2> frames;

    Window window;
    uint32_t currentImageIndex;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::SurfaceKHR surface;

    vk::SwapchainKHR swapchain;
    vk::Format swapchainImageFormat;

    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
    vk::Extent2D swapchainExtent;

    DeletionQueue mainDeletionQueue;

    VmaAllocator allocator;

    VulkanImage drawImage;
    vk::Extent2D drawExtent;

    DescriptorAllocator globalDescriptorAllocator;
    vk::DescriptorSet drawImageDescriptors;
    vk::DescriptorSetLayout drawImageDescriptorLayout;

    vk::Pipeline gradientPipeline;
    vk::PipelineLayout gradientPipelineLayout;

    const std::string windowTitle;
    const glm::uvec2 windowSize;

private:
    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncObjects();
    void InitDescriptors();
    void InitPipelines();

    void InitBackgroundPipelines();
    void CreateSwapchain(glm::ivec2 size);

    inline FrameData& GetCurrentFrame() {
        return frames.at(currentFrame % frames.size());
    }

    // Feel free to copy-paste it into your own code, change it as needed, then call `set_debug_callback()` to use that instead
    static inline VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* userData
    );

    bool isRunning = false;
    bool isInitialized = false;
    bool isRenderingEnabled = true;

    uint32_t currentFrame = 0;
    uint32_t currentSwapchainImageIndex = 0;

    vk::Queue graphicsQueue;
    uint32_t graphicsQueueFamily;

    friend class VulkanImage;
};

}