#pragma once

#include "Lumina/Essence/Utils/NonCopyable.hpp"
#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/Window.hpp"
#include "Lumina/Essence/DeletionQueue.hpp"

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

    const std::string Name;

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


    const std::string windowTitle;
    const glm::ivec2 windowSize;

private:
    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncObjects();

    void CreateSwapchain(glm::ivec2 size);

    inline FrameData& GetCurrentFrame() {
        return frames.at(currentFrame % frames.size());
    }

    static void TransitionImage(vk::CommandBuffer cmd, vk::Image img, vk::ImageLayout srcLayout, vk::ImageLayout dstLayout);
    static vk::ImageSubresourceRange CreateSubresourceRangeForAllLayers(vk::ImageAspectFlags aspect);

    bool IsRunning = false;
    bool IsInitialized = false;
    bool IsRenderingEnabled = true;

    uint32_t currentFrame = 0;
    uint32_t currentSwapchainImageIndex = 0;

    vk::Queue graphicsQueue;
    uint32_t graphicsQueueFamily;
};

}