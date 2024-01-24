#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

#include <VkBootstrap.h>

#include <iostream>
#include <chrono>
#include <thread>

namespace Lumina::Essence {

Application::Application(glm::ivec2 windowSize, std::string const& windowTitle)
    : Name(windowTitle), window(windowSize, windowTitle), windowTitle(windowTitle), windowSize(windowSize) {}
Application::~Application() {
    std::cout << "Application shutting down...\n";

    device.waitIdle();

    for (auto& frame : frames) {
        device.destroyCommandPool(frame.commandPool);
    }

    for (auto imageView : swapchainImageViews) {
        device.destroyImageView(imageView);
    }
    device.destroySwapchainKHR(swapchain);

    instance.destroySurfaceKHR(surface);
    device.destroy();

    instance.destroyDebugUtilsMessengerEXT(debugMessenger);
    instance.destroy();
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";

    InitVulkan();
    InitSwapchain();
    InitCommands();
    InitSyncObjects();

    IsInitialized = true;
}


void Application::InitVulkan() {
    std::cout << "Initializing vulkan\n";
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    vkb::InstanceBuilder builder;
    vkb::Instance vkbInstance = builder.set_app_name(windowTitle.c_str())
                                    .set_app_version(1, 0, 0)
                                    .set_engine_name("Lumina")
                                    .set_engine_version(1, 0, 0)
                                    .request_validation_layers(BuildMode::Current == BuildMode::Debug)
                                    .use_default_debug_messenger()
                                    .require_api_version(1, 3, 0)
                                    .build()
                                    .value();

    instance = vkbInstance.instance;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    debugMessenger = vkbInstance.debug_messenger;

    surface = window.CreateWindowSurface(instance);

    vk::PhysicalDeviceVulkan12Features features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vk::PhysicalDeviceVulkan13Features features13;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    vkb::PhysicalDeviceSelector selector(vkbInstance);
    vkb::PhysicalDevice vkbPhysicalDevice = selector.set_minimum_version(1, 3)
                                                .set_required_features_12(features12)
                                                .set_required_features_13(features13)
                                                .set_surface(surface)
                                                .select()
                                                .value();

    vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
    vkb::Device vkbDevice = deviceBuilder.build().value();

    device = vkbDevice.device;
    physicalDevice = vkbPhysicalDevice.physical_device;
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    std::cout << "Using " << physicalDevice.getProperties().deviceName << "\n";

    std::cout << "Vulkan initialized\n";
}
void Application::InitSwapchain() {
    std::cout << "Initializing swapchain\n";

    CreateSwapchain(windowSize);

    std::cout << "Swapchain initialized\n";
}

void Application::InitCommands() {
    std::cout << "Initializing commands\n";

    vk::CommandPoolCreateInfo commandPoolInfo = {{vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, graphicsQueueFamily};
    for (auto& frame : frames) {
        frame.commandPool = device.createCommandPool(commandPoolInfo);

        frame.mainCommandBuffer = device
                                      .allocateCommandBuffers({
                                          frame.commandPool,                // command pool
                                          vk::CommandBufferLevel::ePrimary, // command buffer level
                                          1,                                // num buffers
                                      })
                                      .at(0);
    }

    std::cout << "Commands initialized\n";
}
void Application::InitSyncObjects() {
    std::cout << "Initializing sync objects\n";
    std::cout << "Sync objects initialized\n";
}

void Application::CreateSwapchain(glm::ivec2 size) {

    swapchainImageFormat = vk::Format::eB8G8R8A8Unorm;

    vkb::SwapchainBuilder builder(physicalDevice, device, surface);
    vkb::Swapchain vkbSwapchain = builder
                                      .set_desired_format(vk::SurfaceFormatKHR{
                                          swapchainImageFormat,
                                          vk::ColorSpaceKHR::eSrgbNonlinear,
                                      })
                                      .set_desired_extent(size.x, size.y)
                                      .add_image_usage_flags(static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eTransferDst))
                                      .build()
                                      .value();
    swapchainExtent = vkbSwapchain.extent;
    swapchain = vkbSwapchain.swapchain;
    auto images = vkbSwapchain.get_images().value();
    for (auto img : images)
        swapchainImages.push_back(img);

    auto imageViews = vkbSwapchain.get_image_views().value();
    for (auto imgView : imageViews)
        swapchainImageViews.push_back(imgView);
}


void Application::Run() {
    if (!IsInitialized)
        throw std::logic_error(
            "Tried running application without initializing it first. Maybe call the parents Initialize() "
            "functions in derived classes."
        );

    IsRunning = true;
    float dt = 1.0f / 60.0f;
    while (IsRunning) {
        while (auto e = window.GetEvent()) {
            HandleEvent(e.value());
        }

        Tick(dt);

        if (!IsRenderingEnabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }


        PreRender(dt);
        Render(dt);
        PostRender(dt);
    }
}
void Application::Exit() {
    IsRunning = false;
}


void Application::Tick(float dt) {}
void Application::PreRender(float dt) {}
void Application::Render(float dt) {}
void Application::PostRender(float dt) {}
void Application::HandleEvent(SDL_Event e) {
    switch (e.type) {
        case SDL_EventType::SDL_EVENT_QUIT:             Exit(); break;
        case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: IsRenderingEnabled = false; break;
        case SDL_EventType::SDL_EVENT_WINDOW_MAXIMIZED: IsRenderingEnabled = true; break;
    }
}

}