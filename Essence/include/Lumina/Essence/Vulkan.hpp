#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

namespace Lumina::Essence {

vk::ImageSubresourceRange CreateSubresourceRangeForAllLayers(vk::ImageAspectFlags aspect);
vk::ShaderModule LoadShaderModule(std::string const& filename, vk::Device device);

}
