#pragma once

#include <vector>

#include "Lumina/Essence/Vulkan.hpp" // IWYU pragma: keep

namespace Lumina::Essence {

class PipelineBuilder {
public:
    void SetShaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader);
    void SetInputTopology(vk::PrimitiveTopology topology);
    void SetPolygonMode(vk::PolygonMode mode);
    void SetCullMode(vk::CullModeFlags cullMode, vk::FrontFace frontFace);
    void DisableBlending();
    void SetMultisamplingNone();
    void SetColorAttachmentFormat(vk::Format format);
    void SetDepthFormat(vk::Format format);
    void DisableDepthTest();
    void SetPipelineLayout(vk::PipelineLayout layout);

    vk::Pipeline Build(vk::Device device);

private:
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    vk::PipelineMultisampleStateCreateInfo multisampling;
    vk::PipelineLayout pipelineLayout;
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineRenderingCreateInfo renderInfo;
    vk::Format colorAttachmentFormat;
};

}