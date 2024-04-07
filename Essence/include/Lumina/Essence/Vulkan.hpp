#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

namespace Lumina::Essence {

vk::ImageSubresourceRange CreateSubresourceRangeForAllLayers(vk::ImageAspectFlags aspect);
vk::ShaderModule LoadShaderModule(std::string const& filename, vk::Device device);

template <typename T>
[[nodiscard]]
inline T VkCheck(vk::ResultValue<T> res) {
    if (res.result != vk::Result::eSuccess) {
        throw std::runtime_error(std::format("Detected Vulkan error: {}\n", to_string(res.result)));
    }
    return res.value;
}
inline void VkCheck(vk::Result res) {
    if (res != vk::Result::eSuccess) {
        throw std::runtime_error(std::format("Detected Vulkan error: {}\n", to_string(res)));
    }
}

}
