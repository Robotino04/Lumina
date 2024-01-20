#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <set>
#include <limits>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"

namespace Lumina::Essence {

Application::Application(glm::ivec2 windowSize, std::string const& windowTitle)
    : Name(windowTitle), window(windowSize, windowTitle) {}
Application::~Application() {
    std::cout << "Application shutting down...\n";

    device.destroyPipelineLayout(pipelineLayout);

    for (auto view : swapchainImageViews) {
        device.destroyImageView(view);
    }

    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    if (debugMessenger.has_value()) instance.destroyDebugUtilsMessengerEXT(debugMessenger.value());
    instance.destroy();
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";
    CreateVulkanInstance();
    CreateVulkanSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
    CreateGraphicsPipeline();

    std::cout << "Vulkan initialized\n";
    IsInitialized = true;
}

void Application::CreateVulkanInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto pprintVersion = [](uint32_t v) {
        return "v" + std::to_string(vk::versionMajor(v)) + "." + std::to_string(vk::versionMinor(v)) + "."
             + std::to_string(vk::versionPatch(v));
    };


    vk::ApplicationInfo appInfo(Name.c_str(), vk::makeVersion(1, 0, 0), "Lumina", vk::makeVersion(1, 0, 0), vk::makeApiVersion(0, 1, 0, 0));


    std::cout << "Initializing Vulkan: \n - " << appInfo.pApplicationName << " "
              << pprintVersion(appInfo.applicationVersion) << "\n - Lumina " << pprintVersion(appInfo.engineVersion)
              << "\n - Vulkan API " << pprintVersion(appInfo.apiVersion) << "\n";

    std::cout << "Available Vulkan extensions: \n";
    auto availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (auto ext : availableExtensions) {
        std::cout << " - " << ext.extensionName << " (" << pprintVersion(ext.specVersion) << ")\n";
    }

    std::cout << "Extensions to load:\n";
    auto requiredExtensions = GetRequiredVulkanExtensions();
    for (auto ext : requiredExtensions) {
        std::cout << " - " << ext << "\n";
    }

    std::cout << "Available Vulkan validation layers: \n";
    auto availableValidationLayers = vk::enumerateInstanceLayerProperties();
    for (auto layer : availableValidationLayers) {
        std::cout << " - " << layer.layerName << " (" << pprintVersion(layer.implementationVersion) << ")\n";
    }

    std::cout << "Validation layers to load:\n";
    auto requiredValidationLayers = GetRequiredVulkanValidationLayers();
    bool hasUnavailableLayers = false;
    for (auto layer : requiredValidationLayers) {
        std::cout << " - " << layer;

        bool isLayerAvailable = false;
        for (auto availableLayer : availableValidationLayers) {
            if (std::string(layer) == std::string(availableLayer.layerName)) {
                isLayerAvailable = true;
                break;
            }
        }
        if (!isLayerAvailable) {
            hasUnavailableLayers = true;
            std::cout << " (not available)";
        }
        std::cout << "\n";
    }
    if (hasUnavailableLayers) {
        throw std::runtime_error("Some validation layers weren't available");
    }

    using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
        {},                                                            // Flags
        eError | eWarning | eVerbose | eInfo,                          // message severity selection
        eDeviceAddressBinding | eGeneral | ePerformance | eValidation, // message type selection
        &Application::vulkanValidationlayerCallback,                   // callback
        nullptr,                                                       // user data

    };
    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceInfo = {
        {{}, &appInfo, requiredValidationLayers, requiredExtensions},
        debugMessengerCreateInfo
    };

    instance = vk::createInstance(instanceInfo.get());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    if (requiredValidationLayers.size()) {
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
    }
}
void Application::CreateVulkanSurface() {
    surface = window.CreateWindowSurface(instance);
}
void Application::PickPhysicalDevice() {
    std::cout << "Available devices:\n";
    auto availableDevices = instance.enumeratePhysicalDevices();
    bool foundDevice = false;
    int32_t maxDeviceScore = -1;

    for (auto dev : availableDevices) {
        int32_t score = ScoreDeviceSuitability(dev);

        if (score > maxDeviceScore) {
            maxDeviceScore = score;
            foundDevice = true;
            physicalDevice = dev;
        }
        std::cout << " - " << dev.getProperties().deviceName << " (" << score << ")\n";
        std::cout << "    - Queue families:\n";
        auto queueFamilies = dev.getQueueFamilyProperties();
        for (auto qfam : queueFamilies) {
            std::cout << "       - " << vk::to_string(qfam.queueFlags) << "\n";
        }

        std::cout << "    - Available device extensions:\n";
        auto availableExtensions = dev.enumerateDeviceExtensionProperties();
        for (auto ext : availableExtensions) {
            std::cout << "       - " << ext.extensionName << "\n";
        }
    }


    std::cout << " - Device extensions to load:\n";
    auto requiredExtensions = GetRequiredVulkanDeviceExtensions();
    for (auto ext : requiredExtensions) {
        std::cout << "    - " << ext << "\n";
    }

    if (!foundDevice) {
        throw std::runtime_error("No suitable device was found");
    }

    std::cout << "Using device " << physicalDevice.getProperties().deviceName << "\n";
}
void Application::CreateLogicalDevice() {
    QueueFamilyIndices indices = GetQueueFamilyIndices(physicalDevice);

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
    };
    if (indices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.presentFamily.value());
    }

    std::vector<float> priorities = {1.0f};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo = {};
    for (auto queueFamily : uniqueQueueFamilies) {
        queueCreateInfo.push_back({{}, queueFamily, priorities});
    }

    vk::PhysicalDeviceFeatures deviceFeatures;

    auto vlayers = GetRequiredVulkanValidationLayers();
    auto exts = GetRequiredVulkanDeviceExtensions();
    vk::DeviceCreateInfo deviceCreateInfo({}, queueCreateInfo, vlayers, exts);

    device = physicalDevice.createDevice(deviceCreateInfo);

    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}
void Application::CreateSwapchain() {
    QueueFamilyIndices indices = GetQueueFamilyIndices(physicalDevice);
    SwapChainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice);
    uint32_t imageCount;
    if (swapchainSupport.capabilities.maxImageCount == 0)
        imageCount = swapchainSupport.capabilities.minImageCount + 1;
    else
        imageCount = std::min(swapchainSupport.capabilities.minImageCount + 1, swapchainSupport.capabilities.maxImageCount);

    auto surfaceFormat = ChooseSwapchainSurfaceFormat(swapchainSupport.formats);
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = ChooseSwapchainExtent(swapchainSupport.capabilities);

    bool needsSharing = indices.graphicsFamily.value() != indices.presentFamily.value();

    std::vector<uint32_t> queueIndices = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (!needsSharing) {
        queueIndices = {};
    }
    vk::SharingMode sharingMode = needsSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;

    vk::SwapchainCreateInfoKHR createInfo = {
        {},                                                        // flags
        surface,                                                   // surface
        imageCount,                                                // num images
        swapchainImageFormat,                                      // image format
        surfaceFormat.colorSpace,                                  // color space
        swapchainExtent,                                           // size
        1,                                                         // num image layers
        vk::ImageUsageFlagBits::eColorAttachment,                  // use for rendering
        sharingMode,                                               // sharing mode
        queueIndices,                                              // queues
        swapchainSupport.capabilities.currentTransform,            // transformations
        vk::CompositeAlphaFlagBitsKHR::eOpaque,                    // how to handle alpha channel
        ChooseSwapchainPresentMode(swapchainSupport.presentModes), // present mode
        vk::True,                                                  // enable clipping
        nullptr                                                    // previous swapchain
    };

    std::cout << "Creating swapchain\n";
    swapchain = device.createSwapchainKHR(createInfo);

    swapchainImages = device.getSwapchainImagesKHR(swapchain);
}
void Application::CreateImageViews() {
    std::cout << "Creating image views\n";
    swapchainImageViews.reserve(swapchainImages.size());

    for (auto const& image : swapchainImages) {
        vk::ImageViewCreateInfo createInfo = {
            {}, // flags
            image,
            vk::ImageViewType::e2D,
            swapchainImageFormat,
            {
             vk::ComponentSwizzle::eIdentity, // red
                vk::ComponentSwizzle::eIdentity, // green
                vk::ComponentSwizzle::eIdentity, // blue
                vk::ComponentSwizzle::eIdentity, // alpha
            },
            {
             vk::ImageAspectFlagBits::eColor, // aspect mask
                0,                               // base mip level
                1,                               // mip level count
                0,                               // base array layer
                1,                               // array layer count
            },
        };

        swapchainImageViews.push_back(device.createImageView(createInfo));
    }
}


bool Application::DeviceSupportsAllExtensions(vk::PhysicalDevice dev) const {
    auto availableExtensions = dev.enumerateDeviceExtensionProperties();
    auto requiredExtensions = GetRequiredVulkanDeviceExtensions();
    for (auto layer : requiredExtensions) {
        bool isExtensionAvailable = false;
        for (auto availableExtension : availableExtensions) {
            if (std::string(layer) == std::string(availableExtension.extensionName)) {
                isExtensionAvailable = true;
                break;
            }
        }
        if (!isExtensionAvailable) {
            return false;
        }
    }
    return true;
}


int Application::ScoreDeviceSuitability(vk::PhysicalDevice dev) const {
    auto deviceProperties = dev.getProperties();

    if (!DeviceSupportsAllExtensions(dev)) return -1;
    if (!GetQueueFamilyIndices(dev).isComplete()) return -1;

    SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(dev);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return -1;

    return 0;
}

Application::QueueFamilyIndices Application::GetQueueFamilyIndices(vk::PhysicalDevice dev) const {
    QueueFamilyIndices queueFamilyIndices;
    auto queueFamilies = dev.getQueueFamilyProperties();
    int i = 0;
    for (auto qfam : queueFamilies) {
        if (qfam.queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilyIndices.graphicsFamily = i;
        }

        if (dev.getSurfaceSupportKHR(i, surface)) {
            queueFamilyIndices.presentFamily = i;
        }

        if (queueFamilyIndices.isComplete()) break;

        i++;
    }
    return queueFamilyIndices;
}

Application::SwapChainSupportDetails Application::QuerySwapchainSupport(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}
vk::SurfaceFormatKHR Application::ChooseSwapchainSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats) const {
    for (auto availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb
            && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats.at(0);
}
vk::PresentModeKHR Application::ChooseSwapchainPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) const {
    for (auto availablePresentMode : availablePresentModes) {
        // use tripple buffering if possible
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    // default to VSync
    return vk::PresentModeKHR::eFifo;
}
vk::Extent2D Application::ChooseSwapchainExtent(vk::SurfaceCapabilitiesKHR const& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    glm::ivec2 fbSize = window.GetFramebufferSize();
    vk::Extent2D extent;
    extent.width =
        glm::clamp<uint32_t>(fbSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height =
        glm::clamp<uint32_t>(fbSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}
void Application::CreateGraphicsPipeline() {
    std::cout << "Creating graphics pipeline\n";
    auto vertexCode = readBinaryFile("resources/shaders/shader.vert.spv");
    auto fragmentCode = readBinaryFile("resources/shaders/shader.frag.spv");

    auto vertexModule = CreateShaderModule(vertexCode);
    auto fragmentModule = CreateShaderModule(fragmentCode);

    vk::PipelineShaderStageCreateInfo vertexCreateInfo = {
        {},
        vk::ShaderStageFlagBits::eVertex,
        vertexModule,
        "main",
    };
    vk::PipelineShaderStageCreateInfo fragmentCreateInfo = {
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragmentModule,
        "main",
    };

    device.destroyShaderModule(vertexModule);
    device.destroyShaderModule(fragmentModule);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        {},                                   // flags
        vk::PrimitiveTopology::eTriangleList, // how to interpret the vertices
        vk::False                             // enable primitive restart
    };

    vk::Viewport viewport = {
        0.0f,                          // x
        0.0f,                          // y
        (float)swapchainExtent.width,  // width
        (float)swapchainExtent.height, // height
        0.0f,                          // min depth
        1.0f,                          // max depth
    };
    vk::Rect2D scissor = {
        {0, 0}, // top left
        swapchainExtent, // size
    };

    // Parameters that can be changed without recreating the pipeline
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {{}, dynamicStates};

    vk::PipelineViewportStateCreateInfo viewportInfo = {
        {},      // flags
        1,       // num viewports
        nullptr, // viewports are set dynamically
        1,       // num scissors
        nullptr, // scissors are set dynamically
    };

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo = {
        {},                          // flags
        vk::False,                   // enable depth clamp
        vk::False,                   // enable discard
        vk::PolygonMode::eFill,      // fill polygons
        vk::CullModeFlagBits::eBack, // cull back faces
        vk::FrontFace::eClockwise,   // triangles are clockwise
        vk::False,                   // enable depth bias
        0.0f,                        // depth bias factor
        0.0f,                        // depth bias clamp
        0.0f,                        // dapth bias slope factor
    };

    vk::PipelineMultisampleStateCreateInfo multisampleInfo = {
        {},                          // flags
        vk::SampleCountFlagBits::e1, // num samples
        vk::False,                   // enable sample shading
        1.0f,                        // min sample shading
        nullptr,                     // sample mask
        vk::False,                   // enable alpha to coverage
        vk::False,                   // enable alpha to one
    };

    using CComponent = vk::ColorComponentFlagBits;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
        vk::False,                          // enable blending
        vk::BlendFactor::eSrcAlpha,         // source color blend factor
        vk::BlendFactor::eOneMinusSrcAlpha, // destination color blend factor
        vk::BlendOp::eAdd,                  // color blend operation
        vk::BlendFactor::eOne,              // source alpha blend factor
        vk::BlendFactor::eZero,             // destination alpha blend factor
        vk::BlendOp::eAdd,                  // alpha blend factor
        CComponent::eR | CComponent::eG | CComponent::eB | CComponent::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendingInfo = {
        {}, // flags
        vk::False, // enable logic operations
        vk::LogicOp::eCopy,
        colorBlendAttachment,
        {
         0.0f, // blending constant/factor 1
            0.0f, // blending constant/factor 2
            0.0f, // blending constant/factor 3
            0.0f, // blending constant/factor 4
        },
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {{}, {}};
    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
}

vk::ShaderModule Application::CreateShaderModule(std::vector<char> const& bytecode) const {
    vk::ShaderModuleCreateInfo createInfo = {
        {},
        bytecode.size(),
        reinterpret_cast<const uint32_t*>(bytecode.data()),
    };

    return device.createShaderModule(createInfo);
}

void Application::Run() {
    if (!IsInitialized)
        throw std::logic_error(
            "Tried running application without initializing it first. Maybe call "
            "the parents Initialize functions in derived classes."
        );

    IsRunning = true;
    while (IsRunning) {
        Tick(1 / 60.0f);
    }
}
void Application::Exit() {
    IsRunning = false;
}


void Application::Tick(float dt) {
    window.PollEvents();

    if (window.ShouldClose()) {
        Exit();
    }
}

std::vector<const char*> Application::GetRequiredVulkanExtensions() const {
    std::vector<const char*> extensions;
    if constexpr (BuildMode::Current == BuildMode::Debug) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

    return extensions;
}

std::vector<const char*> Application::GetRequiredVulkanDeviceExtensions() const {
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    return extensions;
}


std::vector<const char*> Application::GetRequiredVulkanValidationLayers() const {
    if constexpr (BuildMode::Current == BuildMode::Debug) {
        return {
            "VK_LAYER_KHRONOS_validation",
        };
    }
    else {
        return {};
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::vulkanValidationlayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData
) {
    switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) {
        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        case eWarning: std::cerr << "[Vulkan][Warning] " << pCallbackData->pMessage << std::endl; break;
        case eError:   std::cerr << "[Vulkan][Error] " << pCallbackData->pMessage << std::endl; break;
        case eInfo:    break;
        case eVerbose: break;
    }

    return vk::False;
}

}