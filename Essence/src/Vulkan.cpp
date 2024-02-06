#define VMA_IMPLEMENTATION
#include "Lumina/Essence/Vulkan.hpp"

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace Lumina::Essence {

vk::ImageSubresourceRange CreateSubresourceRangeForAllLayers(vk::ImageAspectFlags aspectMask) {
    return {
        aspectMask,
        0,                        // base mip level
        vk::RemainingMipLevels,   // num mip levels
        0,                        // base layer
        vk::RemainingArrayLayers, // num layers
    };
}

}