#include "octahedron.hpp"

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

} //namespace

const uint32_t index_count = sizeof(indices) / sizeof(indices[0]);

VkBuffer getVertexBuffer() { return vk_vertex_buffer; }
VkBuffer getIndexBuffer()  { return vk_index_buffer; }

bool createBuffers(VmaAllocator allocator) {
    const VkBufferCreateInfo vertex_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex)),
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

    std::memcpy(vertex_alloc_result.pMappedData, vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));

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