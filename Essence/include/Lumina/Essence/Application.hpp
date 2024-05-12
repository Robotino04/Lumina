#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/VulkanImage.hpp"
#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/DeletionQueue.hpp"
#include "Lumina/Essence/DescriptorAllocator.hpp"
#include "Lumina/Essence/Utils/Packed.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <array>

namespace Lumina::Essence {

LUMINA_PACKED(struct ComputePushConstants {
    glm::vec4 color1 = {1, 0, 0, 1};
    glm::vec2 samplePoint = {};
});

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

    vk::Fence immediateFence;
    vk::CommandBuffer immediateCommandBuffer;
    vk::CommandPool immediateCommandPool;

    std::array<FrameData, 2> frames;

    Window window;
    uint32_t currentImageIndex = 0;

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

    vk::Pipeline trianglePipeline;
    vk::PipelineLayout trianglePipelineLayout;

    const std::string windowTitle;
    const glm::uvec2 windowSize;

    float time = 0;

private:
    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncObjects();
    void InitDescriptors();
    void InitPipelines();
    void InitImgui();

    void InitBackgroundPipelines();
    void InitTrianglePipeline();
    void CreateSwapchain(glm::ivec2 size);

    void RenderImGui(vk::CommandBuffer cmd, vk::ImageView targetView);

    inline FrameData& GetCurrentFrame() {
        return frames.at(currentFrame % frames.size());
    }

    void SubmitImmediately(std::function<void(vk::CommandBuffer)>&& func);

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