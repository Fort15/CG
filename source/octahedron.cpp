#include "octahedron.hpp"

#include <cstring>
#include <iostream>

namespace octahedron {
const Vertex vertices[] = {
    { { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f, 0.0f} },
    { { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f, 0.0f} },
    { { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f, 1.0f} },
    { {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f, 0.0f} },
    { { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f, 1.0f} },
    { { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f, 1.0f} },
};

const uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);

const uint32_t indices[] = {
    0, 2, 4,
    0, 4, 3,
    0, 3, 5,
    0, 5, 2,

    1, 4, 2,
    1, 3, 4,
    1, 5, 3,
    1, 2, 5,
};

const uint32_t index_count = sizeof(indices) / sizeof(indices[0]);



namespace {
VkBuffer vk_vertex_buffer;
VmaAllocation vk_vertex_buffer_allocation;
VkBuffer vk_index_buffer;
VmaAllocation vk_index_buffer_allocation;
} // namespace

VkBuffer getVertexBuffer() { return vk_vertex_buffer; }
VkBuffer getIndexBuffer()  { return vk_index_buffer; }

bool createBuffers(VmaAllocator allocator) {
    const VkBufferCreateInfo vertex_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo vertex_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo vertex_alloc_result{};
    if (vmaCreateBuffer(allocator, &vertex_buffer_info, &vertex_alloc_info,
                        &vk_vertex_buffer, &vk_vertex_buffer_allocation,
                        &vertex_alloc_result) != VK_SUCCESS) {
        std::cerr << "Failed to create vertex buffer\n";
        return false;
    }

    std::memcpy(vertex_alloc_result.pMappedData, vertices, sizeof(vertices));

    const VkBufferCreateInfo index_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(indices),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo index_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo index_alloc_result{};
    if (vmaCreateBuffer(allocator, &index_buffer_info, &index_alloc_info,
                        &vk_index_buffer, &vk_index_buffer_allocation,
                        &index_alloc_result) != VK_SUCCESS) {
        std::cerr << "Failed to create index buffer\n";
        return false;
    }

    std::memcpy(index_alloc_result.pMappedData, indices, sizeof(indices));

    return true;
}

void destroyBuffers(VmaAllocator allocator) {
    if (vk_index_buffer) {
        vmaDestroyBuffer(allocator, vk_index_buffer, vk_index_buffer_allocation);
        vk_index_buffer = VK_NULL_HANDLE;
    }
    if (vk_vertex_buffer) {
        vmaDestroyBuffer(allocator, vk_vertex_buffer, vk_vertex_buffer_allocation);
        vk_vertex_buffer = VK_NULL_HANDLE;
    }
}

} // namespace octahedron