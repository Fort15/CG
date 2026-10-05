#include "uniforms.hpp"

#include <iostream>

#include "graphics_internal.hpp"

namespace uniforms {

namespace {

VkBuffer vk_uniform_buffers[MAX_OBJECTS] = {};
VmaAllocation vk_uniform_allocations[MAX_OBJECTS] = {};
GlobalUniforms* vk_uniform_memories[MAX_OBJECTS] = {};

VkDescriptorSetLayout vk_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorSet vk_descriptor_sets[MAX_OBJECTS] = {};

} // namespace

VkDescriptorSetLayout getDescriptorSetLayout() { return vk_descriptor_set_layout; }

VkDescriptorSet getDescriptorSet(uint32_t index) {
    return vk_descriptor_sets[index];
}

GlobalUniforms* getMappedMemory(uint32_t index) {
    return vk_uniform_memories[index];
}

bool create(VmaAllocator allocator) {
    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        const VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(GlobalUniforms),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        const VmaAllocationCreateInfo alloc_info = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VmaAllocationInfo alloc_result{};
        if (vmaCreateBuffer(allocator, &buffer_info, &alloc_info,
                            &vk_uniform_buffers[i], &vk_uniform_allocations[i],
                            &alloc_result) != VK_SUCCESS) {
            std::cerr << "Failed to create uniform buffer #" << i << "\n";
            return false;
        }
        vk_uniform_memories[i] = static_cast<GlobalUniforms*>(alloc_result.pMappedData);
    }

    const VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    const VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding,
    };

    if (vkCreateDescriptorSetLayout(graphics::internal::context.device, &layout_info,
                                    nullptr, &vk_descriptor_set_layout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout\n";
        return false;
    }

    const VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = MAX_OBJECTS,
    };

    const VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_OBJECTS,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
    };

    if (vkCreateDescriptorPool(graphics::internal::context.device, &pool_info,
                               nullptr, &vk_descriptor_pool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool\n";
        return false;
    }

    VkDescriptorSetLayout layouts[MAX_OBJECTS];
    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        layouts[i] = vk_descriptor_set_layout;
    }

    const VkDescriptorSetAllocateInfo set_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vk_descriptor_pool,
        .descriptorSetCount = MAX_OBJECTS,
        .pSetLayouts = layouts,
    };

    if (vkAllocateDescriptorSets(graphics::internal::context.device, &set_info,
                                 vk_descriptor_sets) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor sets\n";
        return false;
    }

    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        const VkDescriptorBufferInfo buffer_descriptor = {
            .buffer = vk_uniform_buffers[i],
            .offset = 0,
            .range = sizeof(GlobalUniforms),
        };

        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vk_descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer_descriptor,
        };

        vkUpdateDescriptorSets(graphics::internal::context.device, 1, &write, 0, nullptr);
    }

    return true;
}

void destroy(VmaAllocator allocator) {
    if (vk_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(graphics::internal::context.device, vk_descriptor_pool, nullptr);
        vk_descriptor_pool = VK_NULL_HANDLE;
    }
    if (vk_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(graphics::internal::context.device,
                                     vk_descriptor_set_layout, nullptr);
        vk_descriptor_set_layout = VK_NULL_HANDLE;
    }
    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        if (vk_uniform_buffers[i] != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, vk_uniform_buffers[i], vk_uniform_allocations[i]);
            vk_uniform_buffers[i] = VK_NULL_HANDLE;
            vk_uniform_memories[i] = nullptr;
        }
    }
}

} // namespace uniforms