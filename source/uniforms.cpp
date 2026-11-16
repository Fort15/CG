#include "uniforms.hpp"

#include <iostream>

#include "graphics_internal.hpp"

namespace uniforms {

namespace {
VkBuffer spot_lights_buffer = VK_NULL_HANDLE;
VmaAllocation spot_lights_alloc = VK_NULL_HANDLE;
SpotLightGPU* spot_lights_mem = nullptr;

VkBuffer point_lights_buffer = VK_NULL_HANDLE;
VmaAllocation point_lights_alloc = VK_NULL_HANDLE;
PointLightGPU* point_lights_mem = nullptr;

VkBuffer scene_buffer = VK_NULL_HANDLE;
VmaAllocation scene_alloc = VK_NULL_HANDLE;
SceneUniforms* scene_mem = nullptr;

VkBuffer model_buffer = VK_NULL_HANDLE;
VmaAllocation model_alloc = VK_NULL_HANDLE;
ModelUniforms* model_mem = nullptr;

VkDeviceSize model_stride = 0;

VkDescriptorSetLayout scene_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout model_layout = VK_NULL_HANDLE;
VkDescriptorPool pool = VK_NULL_HANDLE;

VkDescriptorSet scene_set = VK_NULL_HANDLE;
VkDescriptorSet model_sets[MAX_OBJECTS] = {};

VkDeviceSize align_up(VkDeviceSize value, VkDeviceSize alignment) {
    return ((value + alignment - 1) / alignment) * alignment;
}

bool create_buffer(VmaAllocator allocator, VkDeviceSize size, VkBuffer* out_buf, VmaAllocation* out_alloc, void** out_mapped) {
    const VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo result{};
    if (vmaCreateBuffer(allocator, &info, &alloc_info, out_buf, out_alloc, &result) != VK_SUCCESS) {
        return false;
    }
    *out_mapped = result.pMappedData;
    return true;
}

VkDescriptorSetLayout create_scene_layout() {
    const VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    const VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 3,
        .pBindings = bindings,
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(graphics::internal::context.device, &info, nullptr, &layout);
    return layout;
}

VkDescriptorSetLayout create_model_layout() {
    const VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                          VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };

    const VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = bindings,
    };

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;

    vkCreateDescriptorSetLayout(graphics::internal::context.device, &info, nullptr, &layout);

    return layout;
}

} // namespace

VkDescriptorSetLayout getSceneLayout() { return scene_layout; }
VkDescriptorSetLayout getModelLayout() { return model_layout; }
VkDescriptorSet getSceneSet() { return scene_set; }
VkDescriptorSet getModelSet(uint32_t i) { return model_sets[i]; }
SceneUniforms* getSceneMemory() { return scene_mem; }

ModelUniforms* getModelMemory(uint32_t i) {
    if (i >= MAX_OBJECTS || model_mem == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<ModelUniforms*>(reinterpret_cast<char*>(model_mem) + i * model_stride);
}

uint32_t getModelDynamicOffset(uint32_t i) {
    if (i >= MAX_OBJECTS) {
        return 0;
    }

    return static_cast<uint32_t>(i * model_stride);
}

PointLightGPU* getPointLightsMemory() { return point_lights_mem; }
SpotLightGPU* getSpotLightsMemory() { return spot_lights_mem; }

bool create(VmaAllocator allocator) {
    if (!create_buffer(allocator, sizeof(SceneUniforms), &scene_buffer, &scene_alloc,
                       reinterpret_cast<void**>(&scene_mem))) {
        std::cerr << "Failed to create scene uniform buffer\n";
        return false;
    }

    VkPhysicalDeviceProperties properties{};

    vkGetPhysicalDeviceProperties(graphics::internal::context.physical_device, &properties);

    model_stride = align_up(sizeof(ModelUniforms), properties.limits.minUniformBufferOffsetAlignment);

    const VkDeviceSize model_buffer_size = model_stride * MAX_OBJECTS;

    if (!create_buffer(allocator, model_buffer_size, &model_buffer,
                    &model_alloc, reinterpret_cast<void**>(&model_mem))) {
        std::cerr << "Failed to create dynamic model uniform buffer\n";
        return false;
    }

    const VkBufferCreateInfo pl_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(PointLightGPU) * MAX_POINT_LIGHTS,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo pl_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo pl_result{};
    if (vmaCreateBuffer(allocator, &pl_info, &pl_alloc_info,
                        &point_lights_buffer, &point_lights_alloc,
                        &pl_result) != VK_SUCCESS) {
        std::cerr << "Failed to create point lights storage buffer\n";
        return false;
    }
    point_lights_mem = static_cast<PointLightGPU*>(pl_result.pMappedData);

    const VkBufferCreateInfo sl_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(SpotLightGPU) * MAX_SPOT_LIGHTS,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo sl_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo sl_result{};
    if (vmaCreateBuffer(allocator, &sl_info, &sl_alloc_info,
                        &spot_lights_buffer, &spot_lights_alloc,
                        &sl_result) != VK_SUCCESS) {
        std::cerr << "Failed to create spot lights storage buffer\n";
        return false;
    }
    spot_lights_mem = static_cast<SpotLightGPU*>(sl_result.pMappedData);


    scene_layout = create_scene_layout();
    model_layout = create_model_layout();
    if (scene_layout == VK_NULL_HANDLE || model_layout == VK_NULL_HANDLE) {
        std::cerr << "Failed to create descriptor set layouts\n";
        return false;
    }


    const VkDescriptorPoolSize pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = MAX_OBJECTS,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_OBJECTS,
        },
    };

    const VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1 + MAX_OBJECTS,
        .poolSizeCount = 4,
        .pPoolSizes = pool_sizes,
    };
    if (vkCreateDescriptorPool(graphics::internal::context.device, &pool_info,
                               nullptr, &pool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool\n";
        return false;
    }


    const VkDescriptorSetAllocateInfo scene_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &scene_layout,
    };
    if (vkAllocateDescriptorSets(graphics::internal::context.device, &scene_alloc_info,
                                 &scene_set) != VK_SUCCESS) {
        std::cerr << "Failed to allocate scene set\n";
        return false;
    }

    VkDescriptorBufferInfo scene_buf_info = {
        .buffer = scene_buffer, .offset = 0, .range = sizeof(SceneUniforms),
    };
    VkWriteDescriptorSet scene_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = scene_set, .dstBinding = 0, .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &scene_buf_info,
    };
    vkUpdateDescriptorSets(graphics::internal::context.device, 1, &scene_write, 0, nullptr);
    VkDescriptorBufferInfo pl_buf_info = {
        .buffer = point_lights_buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet pl_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = scene_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &pl_buf_info,
    };
    vkUpdateDescriptorSets(graphics::internal::context.device, 1, &pl_write, 0, nullptr);

    VkDescriptorBufferInfo sl_buf_info = {
        .buffer = spot_lights_buffer, .offset = 0, .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet sl_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = scene_set, .dstBinding = 2, .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &sl_buf_info,
    };
    vkUpdateDescriptorSets(graphics::internal::context.device, 1, &sl_write, 0, nullptr);



    VkDescriptorSetLayout layouts[MAX_OBJECTS];
    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) layouts[i] = model_layout;

    const VkDescriptorSetAllocateInfo model_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = MAX_OBJECTS,
        .pSetLayouts = layouts,
    };
    if (vkAllocateDescriptorSets(graphics::internal::context.device, &model_alloc_info,
                                 model_sets) != VK_SUCCESS) {
        std::cerr << "Failed to allocate model sets\n";
        return false;
    }

    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        VkDescriptorBufferInfo buf_info = {
            .buffer = model_buffer,
            .offset = 0,
            .range = sizeof(ModelUniforms),
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = model_sets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &buf_info,
        };

        vkUpdateDescriptorSets(graphics::internal::context.device, 1, &write, 0, nullptr);
    }

    return true;
}

void setTexture(const textures::Texture& texture) {
    const VkDescriptorImageInfo image_info = {
        .sampler = texture.sampler,
        .imageView = texture.image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = model_sets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info,
        };

        vkUpdateDescriptorSets(graphics::internal::context.device, 1, &write, 0, nullptr);
    }
}

void destroy(VmaAllocator allocator) {
    if (pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(graphics::internal::context.device, pool, nullptr);
        pool = VK_NULL_HANDLE;
    }
    if (scene_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(graphics::internal::context.device, scene_layout, nullptr);
        scene_layout = VK_NULL_HANDLE;
    }
    if (model_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(graphics::internal::context.device, model_layout, nullptr);
        model_layout = VK_NULL_HANDLE;
    }
    if (point_lights_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, point_lights_buffer, point_lights_alloc);
        point_lights_buffer = VK_NULL_HANDLE;
        point_lights_mem = nullptr;
    }
    if (spot_lights_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, spot_lights_buffer, spot_lights_alloc);
        spot_lights_buffer = VK_NULL_HANDLE;
        spot_lights_mem = nullptr;
    }
    if (scene_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, scene_buffer, scene_alloc);
        scene_buffer = VK_NULL_HANDLE;
        scene_mem = nullptr;
    }
    if (model_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, model_buffer, model_alloc);

        model_buffer = VK_NULL_HANDLE;
        model_mem = nullptr;
        model_stride = 0;
    }
}

} // namespace uniforms