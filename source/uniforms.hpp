#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <vector>

namespace uniforms {

constexpr uint32_t MAX_OBJECTS = 4;

struct GlobalUniforms {
    glm::mat4 mvp;
    glm::vec4 color;
};

bool create(VmaAllocator allocator);
void destroy(VmaAllocator allocator);

VkDescriptorSetLayout getDescriptorSetLayout();

VkDescriptorSet getDescriptorSet(uint32_t index);

GlobalUniforms* getMappedMemory(uint32_t index);

} // namespace uniforms