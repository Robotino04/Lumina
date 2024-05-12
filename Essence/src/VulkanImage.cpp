#include "Lumina/Essence/VulkanImage.hpp"
#include "Lumina/Essence/Application.hpp"

#include <iostream>

namespace Lumina::Essence {

VulkanImage::VulkanImage(Application& app, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent, vk::ImageAspectFlags aspectFlags) {
    this->imageFormat = format;
    this->imageExtent = extent;
    this->app = &app;

    vk::ImageCreateInfo imageInfo = {
        {},                          // flags
        vk::ImageType::e2D,          // image tpe
        format,                      // format
        extent,                      // size
        1,                           // mip levels
        1,                           // array layers
        vk::SampleCountFlagBits::e1, // num samples
        vk::ImageTiling::eOptimal,   // image tiling
        usageFlags,                  // usage flags
    };
    VkImageCreateInfo oldImageInfo = imageInfo;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlagBits::eDeviceLocal);

    allocation = {};
    VkImage oldImage = nullptr;
    vmaCreateImage(app.allocator, &oldImageInfo, &allocInfo, &oldImage, &allocation, nullptr);
    image = oldImage;

    vk::ImageViewCreateInfo viewInfo = {
        {},                                              // flags
        image,                                           // image
        vk::ImageViewType::e2D,                          // view type
        format,                                          // image format
        {},                                              // component mapping
        CreateSubresourceRangeForAllLayers(aspectFlags), // subresource range
    };

    imageView = app.device.createImageView(viewInfo);

    destroyed = false;
}
VulkanImage::VulkanImage() {
    image = vk::Image{};
    imageView = vk::ImageView{};
    imageExtent = vk::Extent3D{};
    imageFormat = {};
    allocation = {};

    app = nullptr;
    destroyed = true;
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept { // NOLINT(cppcoreguidelines-pro-type-member-init) it does initialize everything
    *this = std::move(other);
}
VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept {
    if (!destroyed) {
        Destroy();
    }

    image = other.image;
    other.image = vk::Image{};
    imageView = other.imageView;
    other.imageView = vk::ImageView{};
    imageExtent = other.imageExtent;
    other.imageExtent = vk::Extent3D{};
    imageFormat = other.imageFormat;
    other.imageFormat = {};
    allocation = other.allocation;
    other.allocation = {};

    app = other.app;
    other.app = nullptr;

    destroyed = other.destroyed;
    other.destroyed = true;

    return *this;
}


VulkanImage::~VulkanImage() {
    if (!destroyed) {
        Destroy();
    }
}

void VulkanImage::Destroy() {
    if (destroyed) {
        std::cerr << "[Vulkan][Error] Tried destroying image twice\n";
        return;
    }

    app->device.destroyImageView(imageView);
    vmaDestroyImage(app->allocator, image, allocation); // calls device.destroyImage() internally

    app = nullptr;
    destroyed = true;
}


void VulkanImage::Blit(vk::CommandBuffer cmd, vk::Image source, vk::Image target, vk::Extent2D sourceSize, vk::Extent2D targetSize) {
    // clang-format off
    vk::ImageBlit2 blitRegion = {
        vk::ImageSubresourceLayers{
            vk::ImageAspectFlagBits::eColor, // aspect mask
            0,                               // base mip level
            0,                               // base array layer
            1,                               // num array layers
        }, // source subresource
        {
            vk::Offset3D{},
            vk::Offset3D{
                static_cast<int32_t>(sourceSize.width),
                static_cast<int32_t>(sourceSize.height),
                1,
            },
        }, // source offsets

        vk::ImageSubresourceLayers{
            vk::ImageAspectFlagBits::eColor, // aspect mask
            0,                               // base mip level
            0,                               // base array layer
            1,                               // num array layers
        }, // destination subresource
        {
            vk::Offset3D{},
            vk::Offset3D{
                static_cast<int32_t>(targetSize.width),
                static_cast<int32_t>(targetSize.height),
                1,
            },
        }, // destination offsets
    };
    // clang-format on

    vk::BlitImageInfo2 blitInfo = {
        source,                               // source image
        vk::ImageLayout::eTransferSrcOptimal, // source layout
        target,                               // destination image
        vk::ImageLayout::eTransferDstOptimal, // destination layout
        blitRegion,                           // region
        vk::Filter::eLinear,                  // sampling filter
    };

    cmd.blitImage2(blitInfo);
}


void VulkanImage::Transition(vk::CommandBuffer cmd, vk::Image img, vk::ImageLayout srcLayout, vk::ImageLayout dstLayout) {
    vk::ImageAspectFlags aspectMask = (dstLayout == vk::ImageLayout::eDepthAttachmentOptimal)
                                        ? vk::ImageAspectFlagBits::eDepth
                                        : vk::ImageAspectFlagBits::eColor;

    // clang-format off
    vk::ImageMemoryBarrier2 imageBarrier = {
        vk::PipelineStageFlagBits2::eAllCommands,                             // source stage mask
        vk::AccessFlagBits2::eMemoryWrite,                                    // source access mask
        vk::PipelineStageFlagBits2::eAllCommands,                             // destination stage mask
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead, // destination access mask
        srcLayout,                                                            // current layout
        dstLayout,                                                            // new layout
        0,                                                                    // source queue family index
        0,                                                                    // dest queue family index
        img,
        CreateSubresourceRangeForAllLayers(aspectMask)
    };
    // clang-format on

    vk::DependencyInfo dependencyInfo = {
        {},           // flags
        nullptr,      // memory barriers
        nullptr,      // buffer barriers
        imageBarrier, // image barriers
    };

    cmd.pipelineBarrier2(dependencyInfo);
}

}