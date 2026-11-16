#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace pipeline {

struct TextureEffectPushConstants {
    uint32_t enabled = 0;
    float time = 0.0f;
    float strength = 0.02f;
    float speed = 1.0f;
};

static_assert(sizeof(TextureEffectPushConstants) == 16);

bool create(VkRenderPass render_pass);
void destroy();

VkPipeline getPipeline();
VkPipelineLayout getLayout();

} // namespace pipeline