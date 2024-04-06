#pragma once

#include <cstdint>
#include <vector>
#include "Lumina/Essence/Vulkan.hpp"


namespace Lumina::Essence {
class DescriptorLayoutBuilder {
public:
    void AddBinding(uint32_t binding, vk::DescriptorType type) {
        vk::DescriptorSetLayoutBinding newBinding = {
            binding,
            type,
            1,
        };
        bindings.push_back(newBinding);
    }

    [[nodiscard]]
    vk::DescriptorSetLayout build(vk::Device device, vk::ShaderStageFlags shaderStages) {
        for (auto& b : bindings) {
            b.stageFlags |= shaderStages;
        }

        vk::DescriptorSetLayoutCreateInfo info = {
            {},
            bindings,
        };

        return device.createDescriptorSetLayout(info);
    }

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

}