#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/PipelineBuilder.hpp"
#include "Lumina/Essence/DescriptorLayoutBuilder.hpp"
#include "Lumina/Essence/Utils/Packed.hpp"

#include <VkBootstrap.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/color_space.hpp>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <iostream>
#include <chrono>
#include <thread>

namespace Lumina::Essence {

Application::Application(glm::ivec2 windowSize, std::string const& windowTitle)
    : name(windowTitle), window(windowSize, windowTitle), windowTitle(windowTitle), windowSize(windowSize) {}
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
    InitDescriptors();
    InitPipelines();
    InitImgui();

    isInitialized = true;
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

        frame.mainCommandBuffer = device.allocateCommandBuffers({
            frame.commandPool,                // command pool
            vk::CommandBufferLevel::ePrimary, // command buffer level
            1,                                // num buffers
        })[0];
        i++;
    }


    immediateCommandPool = device.createCommandPool(commandPoolInfo);
    mainDeletionQueue.PushBack(
        [=, this]() { device.destroyCommandPool(immediateCommandPool); },
        "immediate command pool"
    );
    immediateCommandBuffer = device.allocateCommandBuffers({
        immediateCommandPool,
        vk::CommandBufferLevel::ePrimary,
        1,
    })[0];


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

    immediateFence = device.createFence(fenceInfo);
    mainDeletionQueue.PushBack([&]() { device.destroyFence(immediateFence); }, "immediate fence");

    std::cout << "Sync objects initialized\n";
}
void Application::InitDescriptors() {
    std::cout << "Initializing descriptors\n";

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {vk::DescriptorType::eStorageImage, 1},
    };

    globalDescriptorAllocator.Initialize(device, 10, sizes);
    mainDeletionQueue.PushBack([this]() { globalDescriptorAllocator.Destroy(); }, "global descriptor allocator");

    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, vk::DescriptorType::eStorageImage);
        drawImageDescriptorLayout = builder.build(device, vk::ShaderStageFlagBits::eCompute);
    }

    drawImageDescriptors = globalDescriptorAllocator.Allocate(drawImageDescriptorLayout);

    vk::DescriptorImageInfo imgInfo = {
        {},
        drawImage,
        vk::ImageLayout::eGeneral,
    };

    vk::WriteDescriptorSet drawImageWrite = {
        drawImageDescriptors,
        0,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &imgInfo,
    };

    device.updateDescriptorSets(drawImageWrite, {});

    std::cout << "Descriptors initialized\n";
}


void Application::InitPipelines() {
    std::cout << "Initializing pipelines\n";

    InitBackgroundPipelines();
    InitTrianglePipeline();

    std::cout << "Pipelines initialized\n";
}
void Application::InitImgui() {
    std::array<vk::DescriptorPoolSize, 11> pool_sizes = {
        {
         {vk::DescriptorType::eSampler, 1000},
         {vk::DescriptorType::eCombinedImageSampler, 1000},
         {vk::DescriptorType::eSampledImage, 1000},
         {vk::DescriptorType::eStorageImage, 1000},
         {vk::DescriptorType::eUniformTexelBuffer, 1000},
         {vk::DescriptorType::eStorageTexelBuffer, 1000},
         {vk::DescriptorType::eUniformBuffer, 1000},
         {vk::DescriptorType::eStorageBuffer, 1000},
         {vk::DescriptorType::eUniformBufferDynamic, 1000},
         {vk::DescriptorType::eStorageBufferDynamic, 1000},
         {vk::DescriptorType::eInputAttachment, 1000},
         }
    };

    vk::DescriptorPoolCreateInfo poolInfo = {
        {},
        1000,
        pool_sizes,
    };

    vk::DescriptorPool imguiPool = device.createDescriptorPool(poolInfo);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForVulkan(window.getRawWindow());

    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance = instance,
        .PhysicalDevice = physicalDevice,
        .Device = device,
        .QueueFamily = graphicsQueueFamily,
        .Queue = graphicsQueue,
        .DescriptorPool = imguiPool,
        .RenderPass = nullptr,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfoKHR{{}, swapchainImageFormat},
    };

    ImGui_ImplVulkan_Init(&initInfo);

    mainDeletionQueue.PushBack(
        [=, this]() {
            device.destroyDescriptorPool(imguiPool);
            ImGui_ImplSDL3_Shutdown();
            ImGui_ImplVulkan_Shutdown();
            ImGui::DestroyContext();
        },
        "imgui state"
    );
}


void Application::InitBackgroundPipelines() {
    std::cout << "Initializing background pipelines\n";

    vk::PushConstantRange pushConstants = {
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(ComputePushConstants),
    };

    vk::PipelineLayoutCreateInfo computeLayout = {
        {},
        drawImageDescriptorLayout,
        pushConstants,
    };

    gradientPipelineLayout = device.createPipelineLayout(computeLayout);

    vk::ShaderModule computeDrawShader = LoadShaderModule("resources/shaders/gradient.comp.spv", device);

    vk::PipelineShaderStageCreateInfo stageInfo = {
        {},
        vk::ShaderStageFlagBits::eCompute,
        computeDrawShader,
        "main",
    };

    vk::ComputePipelineCreateInfo computePipelineCreateInfo = {
        {},
        stageInfo,
        gradientPipelineLayout,
    };

    gradientPipeline = VkCheck(device.createComputePipeline(nullptr, computePipelineCreateInfo));

    device.destroyShaderModule(computeDrawShader);

    mainDeletionQueue.PushBack(
        [&]() { device.destroyPipelineLayout(gradientPipelineLayout); },
        "gradient pipeline layout"
    );
    mainDeletionQueue.PushBack([&]() { device.destroyPipeline(gradientPipeline); }, "gradient pipeline");

    std::cout << "Background pipelines initialized\n";
}
void Application::InitTrianglePipeline() {
    std::cout << "Initializing triangle pipeline\n";
    vk::ShaderModule triangleFragmentShader = LoadShaderModule("resources/shaders/colored_triangle.frag.spv", device);
    vk::ShaderModule triangleVertexShader = LoadShaderModule("resources/shaders/colored_triangle.vert.spv", device);

    vk::PipelineLayoutCreateInfo triangleLayout = {
        {},
        drawImageDescriptorLayout,
    };
    trianglePipelineLayout = device.createPipelineLayout(triangleLayout);

    PipelineBuilder builder;
    builder.SetPipelineLayout(trianglePipelineLayout);
    builder.SetShaders(triangleVertexShader, triangleFragmentShader);
    builder.SetInputTopology(vk::PrimitiveTopology::eTriangleList);
    builder.SetPolygonMode(vk::PolygonMode::eFill);
    builder.SetCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);
    builder.SetMultisamplingNone();
    builder.DisableBlending();
    builder.DisableDepthTest();
    builder.SetColorAttachmentFormat(drawImage.GetFormat());
    builder.SetDepthFormat(vk::Format::eUndefined);

    trianglePipeline = builder.Build(device);

    device.destroyShaderModule(triangleFragmentShader);
    device.destroyShaderModule(triangleVertexShader);

    mainDeletionQueue.PushBack(
        [&]() {
            device.destroyPipelineLayout(trianglePipelineLayout);
            device.destroyPipeline(trianglePipeline);
        },
        "triangle pipeline"
    );
    std::cout << "Triangle pipeline initialized\n";
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
                                      .set_desired_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
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

void Application::SubmitImmediately(std::function<void(vk::CommandBuffer)>&& func) {
    device.resetFences(immediateFence);
    immediateCommandBuffer.reset();

    immediateCommandBuffer.begin({{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});

    func(immediateCommandBuffer);

    immediateCommandBuffer.end();

    vk::CommandBufferSubmitInfo submitInfo = {
        immediateCommandBuffer,
    };

    vk::SubmitInfo2 submit = {
        {},
        {},
        submitInfo,
        {},
    };

    graphicsQueue.submit2(submit, immediateFence);
    VkCheck(device.waitForFences(immediateFence, true, UINT64_MAX));
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* userData
) {
    std::string severity = vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity));
    std::string type = vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageType));

    std::cout << std::format("[{}: {}] {}\n", severity, type, pCallbackData->pMessage);

    return vk::False; // Applications must return false here
}

void Application::Run() {
    if (!isInitialized)
        throw std::logic_error(
            "Tried running application without initializing it first. Maybe call the parents Initialize() "
            "functions in derived classes."
        );

    isRunning = true;
    double dt = 1.0f / 60.0f;

    auto lastFrame = std::chrono::high_resolution_clock::now();
    while (isRunning) {
        while (auto e = window.GetEvent()) {
            HandleEvent(e.value());
        }

        Tick(dt);

        if (!isRenderingEnabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }


        PreRender(dt);
        Render(dt);
        PostRender(dt);

        auto thisFrame = std::chrono::high_resolution_clock::now();
        dt = static_cast<std::chrono::duration<double>>(thisFrame - lastFrame).count();
        lastFrame = thisFrame;
        time += dt;
    }
}
void Application::Exit() {
    isRunning = false;
}


void Application::Tick(float dt) {}

void Application::PreRender(float dt) {
    VkCheck(device.waitForFences(GetCurrentFrame().renderFence, true, UINT64_MAX));
    GetCurrentFrame().deletionQueue.Flush();

    device.resetFences(GetCurrentFrame().renderFence);

    currentSwapchainImageIndex =
        device.acquireNextImageKHR(swapchain, UINT64_MAX, GetCurrentFrame().swapchainSemaphore, nullptr).value;

    auto drawImageExtent = drawImage.GetExtent();
    drawExtent = vk::Extent2D{
        drawImageExtent.width,
        drawImageExtent.height,
    };

    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;
    cmd.reset();
    cmd.begin({{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});

    VulkanImage::Transition(cmd, drawImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void Application::Render(float dt) {
    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, gradientPipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, gradientPipelineLayout, 0, drawImageDescriptors, {});

    static ComputePushConstants pc;
    pc.color1 = glm::vec4(glm::rgbColor(glm::hsvColor(pc.color1.xyz()) + glm::vec3(dt * 10, 0, 0)), 1.0);

    const float minAxis = glm::min(windowSize.x, windowSize.y);
    pc.samplePoint = glm::vec2(windowSize) / 2.0f + glm::vec2(minAxis, minAxis) / 4.0f * glm::vec2(cos(time), sin(time));

    ImGui::Begin("Shader Settings");
    ImGui::Text("Time: %f", time);
    ImGui::Text("dT: %f", dt);
    ImGui::ColorEdit3("Color 1", glm::value_ptr(pc.color1));
    ImGui::End();

    cmd.pushConstants(gradientPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pc), &pc);
    cmd.dispatch(std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);


    VulkanImage::Transition(cmd, drawImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment = {
        drawImage,
        vk::ImageLayout::eGeneral,
        vk::ResolveModeFlagBits::eNone,
        {},
        vk::ImageLayout::eGeneral,
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfo renderInfo = CreateRenderingInfo(drawExtent, colorAttachment, nullptr);
    cmd.beginRendering(renderInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, trianglePipeline);

    vk::Viewport viewport = {
        0,
        0,
        static_cast<float>(drawExtent.width),
        static_cast<float>(drawExtent.height),
        0.0f,
        1.0f,
    };

    cmd.setViewport(0, viewport);

    vk::Rect2D scissor = {
        {0, 0},
        drawExtent,
    };

    cmd.setScissor(0, scissor);

    cmd.draw(3, 1, 0, 0);
    cmd.endRendering();
}

void Application::PostRender(float dt) {
    vk::CommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;

    auto& currentImage = swapchainImages[currentSwapchainImageIndex];
    auto& currentImageView = swapchainImageViews[currentSwapchainImageIndex];

    VulkanImage::Transition(cmd, drawImage, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);
    VulkanImage::Transition(cmd, currentImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    VulkanImage::Blit(cmd, drawImage, currentImage, drawExtent, swapchainExtent);

    VulkanImage::Transition(cmd, currentImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    RenderImGui(cmd, currentImageView);
    VulkanImage::Transition(cmd, currentImage, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

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

    VkCheck(graphicsQueue.presentKHR(presentInfo));
    currentFrame++;
}

void Application::RenderImGui(vk::CommandBuffer cmd, vk::ImageView targetView) {
    ImGui::Render();

    vk::RenderingAttachmentInfo colorAttachment = {
        targetView,
        vk::ImageLayout::eGeneral,
        vk::ResolveModeFlagBits::eNone,
        {},
        vk::ImageLayout::eUndefined,
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
    };
    vk::RenderingInfo renderInfo = CreateRenderingInfo(swapchainExtent, colorAttachment, nullptr);

    cmd.beginRendering(renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    cmd.endRendering();
}

void Application::HandleEvent(SDL_Event e) {
    ImGui_ImplSDL3_ProcessEvent(&e);

    switch (e.type) {
        case SDL_EventType::SDL_EVENT_QUIT:             Exit(); break;
        case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED: isRenderingEnabled = false; break;
        case SDL_EventType::SDL_EVENT_WINDOW_RESTORED:  isRenderingEnabled = true; break;
    }
}
}