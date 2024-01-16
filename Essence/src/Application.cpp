#include "Lumina/Essence/Application.hpp"
#include "Lumina/Essence/Platform.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

namespace Lumina::Essence {

Application::Application(std::string const& appName): Name(appName) {}
Application::~Application() {
    if (debugMessenger.has_value()) instance.destroyDebugUtilsMessengerEXT(debugMessenger.value());
    instance.destroy();
}

void Application::Initialize() {
    std::cout << "Initializing Application\n";
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

    std::cout << "Vulkan initialized\n";
    IsInitialized = true;
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

std::vector<const char*> Application::GetRequiredVulkanExtensions() {
    if constexpr (BuildMode::Current == BuildMode::Debug) {
        return {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    }
    else {
        return {};
    }
}

std::vector<const char*> Application::GetRequiredVulkanValidationLayers() {
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