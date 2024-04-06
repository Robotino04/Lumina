#pragma once

#include "Lumina/Essence/Vulkan.hpp"

namespace Lumina::Essence {

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        vk::DescriptorType type;
        float ratio;
    };

    void Initialize(vk::Device device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
        this->device = device;

        std::vector<vk::DescriptorPoolSize> poolSizes;
        for (auto ratio : poolRatios) {
            poolSizes.push_back(vk::DescriptorPoolSize{
                ratio.type,
                uint32_t(ratio.ratio * maxSets),
            });
        }

        vk::DescriptorPoolCreateInfo poolInfo = {
            {},
            maxSets,
            poolSizes,
        };

        pool = device.createDescriptorPool(poolInfo);
    }
    void Destroy() {
        if (device) {
            device.destroyDescriptorPool(pool);
            device = nullptr;
        }
    }

    void Reset() {
        device.resetDescriptorPool(pool, {});
    }

    vk::DescriptorSet Allocate(vk::DescriptorSetLayout layout) {
        vk::DescriptorSetAllocateInfo info = {
            pool,
            1,
            &layout,
        };

        return device.allocateDescriptorSets(info)[0];
    }

private:
    vk::DescriptorPool pool;
    vk::Device device;
};
}