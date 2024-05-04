#include "Lumina/Essence/PipelineBuilder.hpp"

#include <array>

namespace Lumina::Essence {

void PipelineBuilder::SetShaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) {
    shaderStages.clear();
    shaderStages.push_back({
        {},
        vk::ShaderStageFlagBits::eVertex,
        vertexShader,
        "main",
    });
    shaderStages.push_back({
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragmentShader,
        "main",
    });
}
void PipelineBuilder::SetInputTopology(vk::PrimitiveTopology topology) {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = vk::False;
}
void PipelineBuilder::SetPolygonMode(vk::PolygonMode mode) {
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;
}
void PipelineBuilder::SetCullMode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
}
void PipelineBuilder::SetMultisamplingNone() {
    multisampling.sampleShadingEnable = vk::False;

    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;

    multisampling.alphaToCoverageEnable = vk::False;
    multisampling.alphaToOneEnable = vk::False;
}
void PipelineBuilder::DisableBlending() {
    using enum vk::ColorComponentFlagBits;
    colorBlendAttachment.colorWriteMask = eR | eG | eB | eA;
    colorBlendAttachment.blendEnable = vk::False;
}
void PipelineBuilder::SetColorAttachmentFormat(vk::Format format) {
    colorAttachmentFormat = format;

    renderInfo.setColorAttachmentFormats(colorAttachmentFormat);
}
void PipelineBuilder::SetDepthFormat(vk::Format format) {
    renderInfo.depthAttachmentFormat = format;
}
void PipelineBuilder::DisableDepthTest() {
    depthStencil.depthTestEnable = vk::False;
    depthStencil.depthWriteEnable = vk::False;
    depthStencil.depthCompareOp = vk::CompareOp::eNever;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.stencilTestEnable = vk::False;
    depthStencil.front = vk::StencilOpState{};
    depthStencil.back = vk::StencilOpState{};
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
}
void PipelineBuilder::SetPipelineLayout(vk::PipelineLayout layout) {
    pipelineLayout = layout;
}


vk::Pipeline PipelineBuilder::Build(vk::Device device) {
    vk::PipelineViewportStateCreateInfo viewportState = {
        {},
        1,
        nullptr,
        1,
        nullptr,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending = {
        {},
        vk::False,
        vk::LogicOp::eCopy,
        colorBlendAttachment,
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

    std::array<vk::DynamicState, 2> state = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicInfo = {
        {},
        state,
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo = {
        {},
        static_cast<uint32_t>(shaderStages.size()),
        shaderStages.data(),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicInfo,
        pipelineLayout,
    };

    return VkCheck(device.createGraphicsPipeline(nullptr, pipelineInfo));
}
}