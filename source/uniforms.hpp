#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <vector>

#include "textures.hpp"

namespace uniforms {

constexpr uint32_t MAX_OBJECTS = 4;
constexpr uint32_t MAX_TEXTURES = 2;
constexpr uint32_t MAX_POINT_LIGHTS = 8;
constexpr uint32_t MAX_SPOT_LIGHTS = 8;

struct SpotLightGPU {
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 color;
    glm::vec4 params;
};

struct SceneUniforms {
    glm::mat4 view_projection;
    glm::vec4 camera_position;
    glm::vec4 light_direction;
    glm::vec4 light_color;
    glm::vec4 ambient_color;
    glm::uvec4 point_light_count;
    glm::uvec4 spot_light_count;
};

struct ModelUniforms {
    glm::mat4 model;
    glm::vec4 albedo;
    glm::vec4 specular;
    glm::vec4 params;
};

struct PointLightGPU {
    glm::vec4 position;
    glm::vec4 color;
};

bool create(VmaAllocator allocator);
void destroy(VmaAllocator allocator);

VkDescriptorSetLayout getSceneLayout();
VkDescriptorSetLayout getModelLayout();

VkDescriptorSet getSceneSet();
VkDescriptorSet getModelSet(uint32_t index);

SceneUniforms* getSceneMemory();
ModelUniforms* getModelMemory(uint32_t index);
uint32_t getModelDynamicOffset(uint32_t index);

PointLightGPU* getPointLightsMemory();
SpotLightGPU* getSpotLightsMemory();

void setTexture(const textures::Texture& texture, uint32_t texture_index);

} // namespace uniforms