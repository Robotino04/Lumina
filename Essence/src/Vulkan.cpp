#define VMA_IMPLEMENTATION
#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/Utils/FileIO.hpp"

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


vk::RenderingInfo CreateRenderingInfo(vk::Extent2D renderExtent, vk::RenderingAttachmentInfo& colorAttachment, vk::RenderingAttachmentInfo* depthAttachment) {
    return {
        {},
        {{0, 0}, renderExtent},
        1,
        {},
        colorAttachment,
        depthAttachment,
        nullptr,
    };
}

vk::ShaderModule LoadShaderModule(std::string const& filename, vk::Device device) {
    std::vector<uint8_t> bytes = ReadBinaryFile(filename);
    std::vector<uint32_t> buffer(bytes.size() / sizeof(uint32_t));

    std::memcpy(buffer.data(), bytes.data(), bytes.size());

    return device.createShaderModule({{}, buffer});
}

}