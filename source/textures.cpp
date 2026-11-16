#include "textures.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>
#include <iostream>
#include "graphics_internal.hpp"
#include <limits>

namespace textures {

bool loadImage(const std::string& path, ImageData& result) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        std::cerr << "Failed to load texture: " << path << " - " << stbi_failure_reason() << "\n";
        return false;
    }

    const size_t size = static_cast<size_t>(width) * height * 4;

    result.width = width;
    result.height = height;

    result.pixels.assign(pixels, pixels + size);

    stbi_image_free(pixels);

    return true;
}


bool uploadImage(Texture& texture) {
    auto& context = graphics::internal::context;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    const VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = context.graphics_queue_index,
    };

    if (vkCreateCommandPool(context.device, &pool_info,
                            nullptr, &command_pool) != VK_SUCCESS) {
        std::cerr << "Failed to create upload command pool\n";
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

    const VkImageMemoryBarrier to_transfer = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &to_transfer);

    const VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = texture.width,
            .height = texture.height,
            .depth = 1,
        },
    };

    vkCmdCopyBufferToImage(command_buffer, texture.staging_buffer, texture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    const VkImageMemoryBarrier to_shader = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &to_shader);

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

    if (vkQueueSubmit(context.graphics_queue, 1,
                      &submit_info, fence) != VK_SUCCESS) {
        vkDestroyFence(context.device, fence, nullptr);
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }


    if (vkWaitForFences(context.device, 1, &fence,
                        VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        vkQueueWaitIdle(context.graphics_queue);
        vkDestroyFence(context.device, fence, nullptr);
        vkDestroyCommandPool(context.device, command_pool, nullptr);
        return false;
    }

    vkDestroyFence(context.device, fence, nullptr);
    vkDestroyCommandPool(context.device, command_pool, nullptr);

    std::cout << "Image uploaded to GPU\n";
    return true;
}


bool createImage(VmaAllocator allocator, const ImageData& data, Texture& texture) {
    if (data.width <= 0 || data.height <= 0 
        || data.pixels.size() != static_cast<size_t>(data.width) * static_cast<size_t>(data.height) * 4) {
        std::cerr << "Invalid texture image data\n";
        return false;
    }

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width = static_cast<uint32_t>(data.width),
            .height = static_cast<uint32_t>(data.height),
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &texture.image, 
                        &texture.allocation, nullptr) != VK_SUCCESS) {
        std::cerr << "Failed to create GPU image\n";
        return false;          
    }

    const VkDeviceSize image_size = static_cast<VkDeviceSize>(data.pixels.size());

    const VkBufferCreateInfo staging_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = image_size,
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
                        &texture.staging_buffer, &texture.staging_allocation,
                        &staging_alloc_result) != VK_SUCCESS) {
        std::cerr << "Failed to create staging buffer\n";
        destroyImage(allocator, texture);
        return false;
    }

    std::memcpy(staging_alloc_result.pMappedData, data.pixels.data(), static_cast<size_t>(image_size));

    if (vmaFlushAllocation(allocator, texture.staging_allocation, 0, VK_WHOLE_SIZE) != VK_SUCCESS) {
        std::cerr << "Failed to flush staging buffer\n";
        destroyImage(allocator, texture);
        return false;
    }

    texture.width = static_cast<uint32_t>(data.width);
    texture.height = static_cast<uint32_t>(data.height);

    if (!uploadImage(texture)) {
        std::cerr << "Failed to upload image to GPU\n";
        destroyImage(allocator, texture);
        return false;
    }

    const VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = texture.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    if (vkCreateImageView(graphics::internal::context.device, 
                        &view_info, nullptr, &texture.image_view) != VK_SUCCESS) {
        std::cerr << "Failed to create image view\n";
        destroyImage(allocator, texture);
        return false;
    }

    const VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (vkCreateSampler(graphics::internal::context.device, &sampler_info,
                        nullptr, &texture.sampler) != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler\n";
        destroyImage(allocator, texture);
        return false;
    }

    std::cout << "Image view and sampler created\n";

    vmaDestroyBuffer(allocator, texture.staging_buffer, texture.staging_allocation);

    texture.staging_buffer = VK_NULL_HANDLE;
    texture.staging_allocation = VK_NULL_HANDLE;

    std::cout << "GPU image created: " << texture.width << "x" << texture.height << "\n";

    return true;
}

void destroyImage(VmaAllocator allocator, Texture& texture) {
    auto& context = graphics::internal::context;

    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(context.device, texture.sampler, nullptr);
    }

    if (texture.image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(context.device, texture.image_view, nullptr);
    }

    if (texture.staging_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, texture.staging_buffer, texture.staging_allocation);
    }

    if (texture.image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, texture.image, texture.allocation);
    }

    texture = {};
}

}