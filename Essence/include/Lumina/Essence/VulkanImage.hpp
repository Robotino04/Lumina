#pragma once

#include "Lumina/Essence/Vulkan.hpp"
#include "Lumina/Essence/Utils/NonCopyable.hpp"

namespace Lumina::Essence {

class Application;

class VulkanImage : NonCopyable {
public:
    VulkanImage(Application& app, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent, vk::ImageAspectFlags aspectFlags);
    VulkanImage();
    VulkanImage(VulkanImage&& other) noexcept;            // allow moving
    VulkanImage& operator=(VulkanImage&& other) noexcept; // allow moving

    ~VulkanImage();

    inline operator vk::Image() {
        return image;
    }
    inline operator vk::ImageView() const {
        return imageView;
    }

    inline vk::Extent3D GetExtent() const {
        return imageExtent;
    }

    inline vk::Format GetFormat() const {
        return imageFormat;
    }

    void Destroy();

    static void Blit(vk::CommandBuffer cmd, vk::Image source, vk::Image target, vk::Extent2D sourceSize, vk::Extent2D targetSize);
    static void Transition(vk::CommandBuffer cmd, vk::Image img, vk::ImageLayout srcLayout, vk::ImageLayout dstLayout);

private:
    Application* app = nullptr;

    vk::Image image;
    vk::ImageView imageView;
    VmaAllocation allocation;
    vk::Extent3D imageExtent;
    vk::Format imageFormat;

    bool destroyed = true;
};

}