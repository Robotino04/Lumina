#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"

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


    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        &Application::vulkanValidationlayerCallback,
        nullptr
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
    uint32_t imageCount =
        swapchainSupport.capabilities.maxImageCount == 0
            ? swapchainSupport.capabilities.minImageCount + 1
            : std::min(swapchainSupport.capabilities.minImageCount + 1, swapchainSupport.capabilities.maxImageCount);

    auto surfaceFormat = ChooseSwapchainSurfaceFormat(swapchainSupport.formats);
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = ChooseSwapchainExtent(swapchainSupport.capabilities);

    bool needsParallelism = indices.graphicsFamily.value() != indices.presentFamily.value();

    std::vector<uint32_t> queueIndices = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (!needsParallelism) {
        queueIndices = {};
    }

    vk::SwapchainCreateInfoKHR createInfo = {
        {},
        surface,
        imageCount,
        swapchainImageFormat,
        surfaceFormat.colorSpace,
        swapchainExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        needsParallelism ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        queueIndices,
        swapchainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        ChooseSwapchainPresentMode(swapchainSupport.presentModes),
        vk::True,
        nullptr
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
            {},
            image,
            vk::ImageViewType::e2D,
            swapchainImageFormat,
            {
             vk::ComponentSwizzle::eIdentity,
             vk::ComponentSwizzle::eIdentity,
             vk::ComponentSwizzle::eIdentity,
             vk::ComponentSwizzle::eIdentity,
             },
            {
             vk::ImageAspectFlagBits::eColor,
             0, 1,
             0, 1,
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
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: std::cerr << "[Vulkan][Warning] ";
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            std::cerr << "[Vulkan][Error] ";
            std::cerr << pCallbackData->pMessage << std::endl;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: break;
    }

    return vk::False;
}

}