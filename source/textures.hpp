#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>



namespace textures {

struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;
};

struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    VkImageView image_view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VmaAllocation staging_allocation = VK_NULL_HANDLE;

    uint32_t width = 0;
    uint32_t height = 0;
};

bool loadImage(const std::string& path, ImageData& result);
bool createImage(VmaAllocator allocator, const ImageData& data, Texture& texture);
void destroyImage(VmaAllocator allocator, Texture& texture);

}