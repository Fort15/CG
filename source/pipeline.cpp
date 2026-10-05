#include "pipeline.hpp"

#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>

#include "graphics_internal.hpp"
#include "uniforms.hpp"

namespace pipeline {

namespace {

VkPipeline vk_pipeline = VK_NULL_HANDLE;
VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;

VkShaderModule loadShaderModule(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open shader file: " << path << "\n";
        return VK_NULL_HANDLE;
    }

    const size_t size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();

    const VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = buffer.data(),
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(graphics::internal::context.device, &info,
                             nullptr, &module) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module: " << path << "\n";
        return VK_NULL_HANDLE;
    }
    return module;
}

} // namespace

VkPipeline getPipeline() { return vk_pipeline; }
VkPipelineLayout getLayout() { return vk_pipeline_layout; }

bool create(VkRenderPass render_pass) {
    VkShaderModule vert_module = loadShaderModule("shaders/octahedron.vert.spv");
    VkShaderModule frag_module = loadShaderModule("shaders/octahedron.frag.spv");

    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        std::cerr << "Failed to load shaders. Did you run from project root?\n";
        return false;
    }

    const VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main",
        },
    };

    const VkVertexInputBindingDescription vertex_bindings[] = {
        {
            .binding = 0,
            .stride = sizeof(float) * 6,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    const VkVertexInputAttributeDescription vertex_attributes[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = sizeof(float) * 3,
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = vertex_bindings,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_attributes,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    const VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states,
    };

    const VkDescriptorSetLayout descriptor_layout = uniforms::getDescriptorSetLayout();

    const VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
    };
    if (vkCreatePipelineLayout(graphics::internal::context.device, &layout_info,
                               nullptr, &vk_pipeline_layout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout\n";
        return false;
    }

    const VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = vk_pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(graphics::internal::context.device,
                                  VK_NULL_HANDLE, 1, &pipeline_info,
                                  nullptr, &vk_pipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline\n";
        return false;
    }

    vkDestroyShaderModule(graphics::internal::context.device, vert_module, nullptr);
    vkDestroyShaderModule(graphics::internal::context.device, frag_module, nullptr);

    return true;
}

void destroy() {
    if (vk_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(graphics::internal::context.device, vk_pipeline, nullptr);
        vk_pipeline = VK_NULL_HANDLE;
    }
    if (vk_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(graphics::internal::context.device,
                                vk_pipeline_layout, nullptr);
        vk_pipeline_layout = VK_NULL_HANDLE;
    }
}

} // namespace pipeline