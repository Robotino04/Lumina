#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

#include <VkBootstrap.h>
#include <glm/glm.hpp>

#include <iostream>
#include <chrono>
#include <thread>

namespace Lumina::Essence {

Application::Application(glm::ivec2 windowSize, std::string const& windowTitle)
    : Name(windowTitle), window(windowSize, windowTitle), windowTitle(windowTitle), windowSize(windowSize) {}
Application::~Application() {
    std::cout << "Application shutting down...\n";

    device.waitIdle();

    mainDeletionQueue.Flush();
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
                                    .set_debug_callback(VulkanDebugCallback)
                                    .require_api_version(1, 3, 0)
                                    .build()
                                    .value();

    instance = vkbInstance.instance;
    mainDeletionQueue.PushBack([&]() { instance.destroy(); }, "instance");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    debugMessenger = vkbInstance.debug_messenger;
    mainDeletionQueue.PushBack(
        [&]() { instance.destroyDebugUtilsMessengerEXT(debugMessenger); },
        "debug messenger"
    );

    surface = window.CreateWindowSurface(instance);
    mainDeletionQueue.PushBack([&]() { instance.destroySurfaceKHR(surface); }, "surface");

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
    mainDeletionQueue.PushBack([&]() { device.destroy(); }, "logical device");
    physicalDevice = vkbPhysicalDevice.physical_device;
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    std::cout << "Using " << physicalDevice.getProperties().deviceName << "\n";

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);
    mainDeletionQueue.PushBack([&]() { vmaDestroyAllocator(allocator); }, "allocator");

    std::cout << "Vulkan initialized\n";
}
void Application::InitSwapchain() {
    std::cout << "Initializing swapchain\n";

    CreateSwapchain(windowSize);

    vk::Extent3D drawImageExtent = {
        windowSize.x,
        windowSize.y,
        1,
    };

    using enum vk::ImageUsageFlagBits;
    drawImage = VulkanImage(
        *this,
        vk::Format::eR16G16B16A16Sfloat,
        eTransferSrc | eTransferDst | eStorage | eColorAttachment,
        drawImageExtent,
        vk::ImageAspectFlagBits::eColor
    );

    mainDeletionQueue.PushBack([&]() { drawImage.Destroy(); }, "draw image");

    std::cout << "Swapchain initialized\n";
}

void Application::InitCommands() {
    std::cout << "Initializing commands\n";

    vk::CommandPoolCreateInfo commandPoolInfo = {{vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, graphicsQueueFamily};
    int i = 0;
    for (auto& frame : frames) {
        frame.commandPool = device.createCommandPool(commandPoolInfo);

        mainDeletionQueue.PushBack([=, this]() { device.destroyCommandPool(frame.commandPool); }, std::format("command pool F{}", i));

        frame.mainCommandBuffer = device
                                      .allocateCommandBuffers({
                                          frame.commandPool,                // command pool
                                          vk::CommandBufferLevel::ePrimary, // command buffer level
                                          1,                                // num buffers
                                      })
                                      .at(0);
        i++;
    }

    std::cout << "Commands initialized\n";
}
void Application::InitSyncObjects() {
    std::cout << "Initializing sync objects\n";

    vk::SemaphoreCreateInfo semaphoreInfo = {};
    vk::FenceCreateInfo fenceInfo = {{vk::FenceCreateFlagBits::eSignaled}};

    int i = 0;
    for (auto& frame : frames) {
        frame.renderSemaphore = device.createSemaphore(semaphoreInfo);
        frame.swapchainSemaphore = device.createSemaphore(semaphoreInfo);
        frame.renderFence = device.createFence(fenceInfo);

        mainDeletionQueue.PushBack(
            [&, i = i]() {
                device.destroySemaphore(frame.renderSemaphore);
                device.destroySemaphore(frame.swapchainSemaphore);
                device.destroyFence(frame.renderFence);
            },
            std::format("sync objects F{}", i)
        );
        i++;
    }

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
    mainDeletionQueue.PushBack([&]() { device.destroySwapchainKHR(swapchain); }, "swapchain");

    auto images = vkbSwapchain.get_images().value();
    for (auto img : images)
        swapchainImages.push_back(img);

    auto imageViews = vkbSwapchain.get_image_views().value();
    int i = 0;
    for (auto imgView : imageViews) {
        swapchainImageViews.push_back(imgView);
        mainDeletionQueue.PushBack([=, this]() { device.destroyImageView(imgView); }, std::format("image view F{}", i));
        i++;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* userData
) {
    std::string severity;
    switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) {
        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        case eVerbose: severity = "Verbose"; break;
        case eError:   severity = "Error"; break;
        case eWarning: severity = "Warning"; break;
        case eInfo:    severity = "Info"; break;
        default:       severity = "Unknown"; break;
    };

    std::string type;
    const auto castMessageType = static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageType);

    using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
    if (castMessageType & eDeviceAddressBinding) {
        type += "DeviceAddressBinding";
    }
    if (castMessageType & eGeneral) {
        if (!type.empty()) type += " | ";
        type += "General";
    }
    if (castMessageType & eValidation) {
        if (!type.empty()) type += " | ";
        type += "Validation";
    }
    if (castMessageType & ePerformance) {
        if (!type.empty()) type += " | ";
        type += "Performance";
    }

    if (type.empty()) {
        type = "Unknown";
    }

    std::cout << std::format("[{}: {}] {}\n", severity, type, pCallbackData->pMessage);

    return vk::False; // Applications must return false here
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

void Application::PreRender(float dt) {
    device.waitForFences(GetCurrentFrame().renderFence, true, UINT64_MAX);
    GetCurrentFrame().deletionQueue.Flush();

    device.resetFences(GetCurrentFrame().renderFence);

    currentSwapchainImageIndex =
        device.acquireNextImageKHR(swapchain, UINT64_MAX, GetCurrentFrame().swapchainSemaphore, nullptr).value;

    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;
    cmd.reset();
    cmd.begin({{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});

    VulkanImage::Transition(cmd, drawImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
}

void Application::Render(float dt) {
    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;

    // make a clear-color from frame number. This will flash with a 120 frame period.
    float flash = glm::abs(glm::sin(float(currentFrame) / 120.0f));
    vk::ClearColorValue clearValue(0.0f, 0.0f, flash, 1.0f);

    vk::ImageSubresourceRange clearRange = CreateSubresourceRangeForAllLayers(vk::ImageAspectFlagBits::eColor);

    // clear image
    cmd.clearColorImage(drawImage, vk::ImageLayout::eGeneral, clearValue, clearRange);
}

void Application::PostRender(float dt) {
    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;

    VulkanImage::Transition(cmd, drawImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
    VulkanImage::Transition(cmd, swapchainImages[currentSwapchainImageIndex], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    VulkanImage::Blit(cmd, drawImage, swapchainImages[currentSwapchainImageIndex], drawExtent, swapchainExtent);

    VulkanImage::Transition(cmd, swapchainImages[currentSwapchainImageIndex], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

    cmd.end();

    vk::CommandBufferSubmitInfo submitInfo = {cmd};

    vk::SemaphoreSubmitInfo waitInfo = {GetCurrentFrame().swapchainSemaphore, 1, vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput};
    vk::SemaphoreSubmitInfo signalInfo = {GetCurrentFrame().renderSemaphore, 1, vk::PipelineStageFlagBits2KHR::eAllGraphics};

    vk::SubmitInfo2 submit = {{}, waitInfo, submitInfo, signalInfo};

    graphicsQueue.submit2(submit, GetCurrentFrame().renderFence);

    vk::PresentInfoKHR presentInfo = {
        GetCurrentFrame().renderSemaphore,
        swapchain,
        currentSwapchainImageIndex,
    };

    graphicsQueue.presentKHR(presentInfo);
    currentFrame++;
}
void Application::HandleEvent(SDL_Event e) {
    switch (e.type) {
        case SDL_EventType::SDL_EVENT_QUIT:             Exit(); break;
        case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: IsRenderingEnabled = false; break;
        case SDL_EventType::SDL_EVENT_WINDOW_MAXIMIZED: IsRenderingEnabled = true; break;
    }
}

}