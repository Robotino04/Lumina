#pragma once

#include "Lumina/Essence/Vulkan.hpp"

namespace Lumina::Essence {

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        vk::DescriptorType type;
        float ratio;
    };

    DescriptorAllocator(vk::Device device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
        : device(device) {
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
    ~DescriptorAllocator(){
        device.destroyDescriptorPool(pool);
    }

    void reset() {
        device.resetDescriptorPool(pool, {});
    }

private:
    vk::DescriptorPool pool;
    vk::Device device;
};
}