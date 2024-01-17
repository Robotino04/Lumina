#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

namespace Lumina::Essence {

Application::Application(std::string const& appName): Name(appName) {}
Application::~Application() {
    std::cout << "Application shutting down...\n";
    device.destroy();
    if (debugMessenger.has_value()) instance.destroyDebugUtilsMessengerEXT(debugMessenger.value());
    instance.destroy();
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";
    CreateVulkanInstance();
    PickPhysicalDevice();
    CreateLogicalDevice();

    std::cout << "Vulkan initialized\n";
    IsInitialized = true;
}

void Application::CreateVulkanInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto pprintVersion = [](uint32_t v) {
        return "v" + std::to_string(vk::versionMajor(v)) + "." + std::to_string(vk::versionMinor(v)) + "."
             + std::to_string(vk::versionPatch(v));
    };


    vk::ApplicationInfo appInfo(
        Name.c_str(), vk::makeVersion(1, 0, 0), "Lumina", vk::makeVersion(1, 0, 0), vk::makeApiVersion(0, 1, 0, 0)
    );


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
        {
         {},
         &appInfo,
         requiredValidationLayers, requiredExtensions,
         },
        debugMessengerCreateInfo
    };

    instance = vk::createInstance(instanceInfo.get());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    if (requiredValidationLayers.size()) {
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
    }
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
        auto queueFamilies = dev.getQueueFamilyProperties();
        for (auto qfam : queueFamilies) {
            std::cout << "    - " << vk::to_string(qfam.queueFlags) << "\n";
        }
    }
    if (!foundDevice) {
        throw std::runtime_error("No suitable device was found");
    }

    std::cout << "Using device " << physicalDevice.getProperties().deviceName << "\n";
}
void Application::CreateLogicalDevice() {
    QueueFamilyIndices indices = GetQueueFamilyIndices(physicalDevice);

    std::vector<float> priorities = {1.0f};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo = {
        {{}, indices.graphicsFamily.value(), priorities},
    };

    vk::PhysicalDeviceFeatures deviceFeatures;

    auto vlayers = GetRequiredVulkanValidationLayers();
    auto exts = GetRequiredVulkanPerDeviceExtensions();
    vk::DeviceCreateInfo deviceCreateInfo({}, queueCreateInfo, vlayers, exts);
    device = physicalDevice.createDevice(deviceCreateInfo);
    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
}


int Application::ScoreDeviceSuitability(vk::PhysicalDevice dev) const {
    auto deviceProperties = dev.getProperties();

    if (!GetQueueFamilyIndices(dev).isComplete()) return -1;

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

        if (queueFamilyIndices.isComplete()) break;

        i++;
    }
    return queueFamilyIndices;
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


void Application::Tick(float dt) {}

std::vector<const char*> Application::GetRequiredVulkanExtensions() const {
    if constexpr (BuildMode::Current == BuildMode::Debug) {
        return {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    }
    else {
        return {};
    }
}

std::vector<const char*> Application::GetRequiredVulkanPerDeviceExtensions() const {
    return {};
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