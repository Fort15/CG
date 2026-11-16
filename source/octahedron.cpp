#include "octahedron.hpp"
#include "graphics_internal.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace octahedron {

namespace {
const float base_positions[6][3] = {
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, -1.0f, 0.0f},
    { 1.0f, 0.0f, 0.0f},
    { -1.0f, 0.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f},
    { 0.0f, 0.0f, -1.0f}
};

const float base_colors[6][3] = {
    { 1.0f, 0.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f},
    { 1.0f, 1.0f, 0.0f},
    { 1.0f, 0.0f, 1.0f},
    { 0.0f, 1.0f, 1.0f}
};

const uint32_t base_indices[24] = {
    0, 2, 4,
    0, 4, 3,
    0, 3, 5,
    0, 5, 2,

    1, 4, 2,
    1, 3, 4,
    1, 5, 3,
    1, 2, 5,
};


std::vector<Vertex> generate_vertices() {
    std::vector<Vertex> vertices;
    vertices.reserve(24);

    for (int face = 0; face < 8; ++face) {
        uint32_t i0 = base_indices[face * 3];
        uint32_t i1 = base_indices[face * 3 + 1];
        uint32_t i2 = base_indices[face * 3 + 2];

        glm::vec3 p0(base_positions[i0][0], base_positions[i0][1], base_positions[i0][2]);
        glm::vec3 p1(base_positions[i1][0], base_positions[i1][1], base_positions[i1][2]);
        glm::vec3 p2(base_positions[i2][0], base_positions[i2][1], base_positions[i2][2]);

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        glm::vec3 face_center = (p0 + p1 + p2) / 3.0f;

        if (glm::dot(normal, face_center) < 0.0f) {
            std::swap(i1, i2);

            p1 = glm::vec3(base_positions[i1][0], base_positions[i1][1], base_positions[i1][2]);
            p2 = glm::vec3(base_positions[i2][0], base_positions[i2][1], base_positions[i2][2]);

            edge1 = p1 - p0;
            edge2 = p2 - p0;
            normal = glm::cross(edge1, edge2);
        }

        normal = glm::normalize(normal);

        const uint32_t face_indices[3] = {i0, i1, i2};

        const float face_uv[3][2] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {0.5f, 1.0f}
        };

        for (int j = 0; j < 3; ++j) {
            uint32_t idx = face_indices[j];

            Vertex v{};

            for (int k = 0; k < 3; ++k) {
                v.position[k] = base_positions[idx][k];
                v.color[k] = base_colors[idx][k];
            }

            v.normal[0] = normal.x;
            v.normal[1] = normal.y;
            v.normal[2] = normal.z;

            v.uv[0] = face_uv[j][0];
            v.uv[1] = face_uv[j][1];

            vertices.push_back(v);
        }
    }

    return vertices;
}



const std::vector<Vertex> vertices = generate_vertices();

const uint32_t vertex_count = static_cast<uint32_t>(vertices.size());

const uint32_t indices[24] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23
};

VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
VkBuffer vk_index_buffer;
VmaAllocation vk_index_buffer_allocation;

bool copyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size) {
    auto& context = graphics::internal::context;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    const VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = context.graphics_queue_index,
    };

    if (vkCreateCommandPool(context.device, &pool_info, nullptr,
                            &command_pool) != VK_SUCCESS) {
        std::cerr << "Failed to create copy command pool\n";
        return false;
    }

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(context.device, &alloc_info,
                                &command_buffer) != VK_SUCCESS) {
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    const VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    const VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                         VK_ACCESS_INDEX_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = destination,
        .offset = 0,
        .size = size,
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
                        0, nullptr, 1, &barrier, 0, nullptr
    );

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    const VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    if (vkCreateFence(context.device, &fence_info,
                    nullptr, &fence) != VK_SUCCESS) {
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    if (vkQueueSubmit(context.graphics_queue, 1, &submit_info, fence) != VK_SUCCESS) {
        vkDestroyFence(context.device, fence, nullptr);
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    if (vkWaitForFences(context.device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        vkDeviceWaitIdle(context.device);
        vkDestroyFence(context.device, fence, nullptr);
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    vkDestroyFence(context.device, fence, nullptr);
    vkDestroyCommandPool(context.device, command_pool, nullptr);

    return true;
}

bool uploadBuffer(VmaAllocator allocator, const void* data, VkDeviceSize size,
                VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& allocation) {
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;

    const VkBufferCreateInfo staging_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo staging_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo staging_alloc_result{};

    if (vmaCreateBuffer(allocator, &staging_buffer_info, &staging_alloc_info,
                        &staging_buffer, &staging_allocation, &staging_alloc_result) != VK_SUCCESS) {
        std::cerr << "Failed to create staging buffer\n";
        return false;
    }

    std::memcpy(staging_alloc_result.pMappedData, data, static_cast<size_t>(size));

    if (vmaFlushAllocation(allocator, staging_allocation, 0, VK_WHOLE_SIZE) != VK_SUCCESS) {
        std::cerr << "Failed to flush staging buffer\n";
        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
        return false;
    }

    const VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo buffer_alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    if (vmaCreateBuffer(allocator, &buffer_info, &buffer_alloc_info,
                        &buffer, &allocation, nullptr) != VK_SUCCESS) {
        std::cerr << "Failed to create GPU buffer\n";
        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
        return false;
    }

    if (!copyBuffer(staging_buffer, buffer, size)) {
        std::cerr << "Failed to copy buffer to GPU\n";

        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
        vmaDestroyBuffer(allocator, buffer, allocation);

        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;

        return false;
    }

    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);

    return true;
}

} //namespace

const uint32_t index_count = sizeof(indices) / sizeof(indices[0]);

VkBuffer getVertexBuffer() { return vk_vertex_buffer; }
VkBuffer getIndexBuffer()  { return vk_index_buffer; }

bool createBuffers(VmaAllocator allocator) {
    const VkDeviceSize vertex_buffer_size = static_cast<VkDeviceSize>(vertices.size() * sizeof(Vertex));

    if (!uploadBuffer(allocator, vertices.data(), vertex_buffer_size,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vk_vertex_buffer, vk_vertex_buffer_allocation)) {
        std::cerr << "Failed to upload vertex buffer\n";
        destroyBuffers(allocator);
        return false;
    }

    const VkDeviceSize index_buffer_size = sizeof(indices);

    if (!uploadBuffer(allocator, indices, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    vk_index_buffer, vk_index_buffer_allocation)) {
        std::cerr << "Failed to upload index buffer\n";
        destroyBuffers(allocator);
        return false;
    }

    std::cout << "Vertex and index buffers uploaded to GPU\n";

    return true;
}

void destroyBuffers(VmaAllocator allocator) {
    if (vk_index_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, vk_index_buffer, vk_index_buffer_allocation);

        vk_index_buffer = VK_NULL_HANDLE;
        vk_index_buffer_allocation = VK_NULL_HANDLE;
    }

    if (vk_vertex_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, vk_vertex_buffer, vk_vertex_buffer_allocation);

        vk_vertex_buffer = VK_NULL_HANDLE;
        vk_vertex_buffer_allocation = VK_NULL_HANDLE;
    }
}

} // namespace octahedron