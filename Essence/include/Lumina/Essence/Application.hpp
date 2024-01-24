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

    virtual void HandleEvent(SDL_Event e);

    void Run();
    void Exit();

    const std::string Name;

protected:
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


    const std::string windowTitle;
    const glm::ivec2 windowSize;

private:
    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncObjects();

    void CreateSwapchain(glm::ivec2 size);

    bool IsRunning = false;
    bool IsInitialized = false;
    bool IsRenderingEnabled = true;
};

}