#pragma once


#include <cstdint>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include "Lumina/Essence/Vulkan.hpp"


namespace Lumina::Essence {
class DescriptorLayoutBuilder {
public:
    void AddBinding(uint32_t binding, vk::DescriptorType) {}

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

}