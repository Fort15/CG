#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>

namespace octahedron {

struct Vertex {
    float position[3];
    float normal[3];
    float color[3];
};

extern const uint32_t index_count;

bool createBuffers(VmaAllocator allocator);
void destroyBuffers(VmaAllocator allocator);

VkBuffer getVertexBuffer();
VkBuffer getIndexBuffer();

} // namespace octahedron