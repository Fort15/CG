#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>

namespace octahedron {

struct Vertex {
    float position[3];
    float color[3];
};

extern const Vertex vertices[];
extern const uint32_t vertex_count;
extern const uint32_t indices[];
extern const uint32_t index_count;

bool createBuffers(VmaAllocator allocator);
void destroyBuffers(VmaAllocator allocator);

VkBuffer getVertexBuffer();
VkBuffer getIndexBuffer();

} // namespace octahedron