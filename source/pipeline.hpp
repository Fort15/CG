#pragma once

#include <vulkan/vulkan.h>

namespace pipeline {

bool create(VkRenderPass render_pass);

void destroy();

VkPipeline getPipeline();
VkPipelineLayout getLayout();

} // namespace pipeline