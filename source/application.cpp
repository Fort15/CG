#include "application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include "octahedron.hpp"
#include "pipeline.hpp"
#include "uniforms.hpp"

#include "textures.hpp"
#include "graphics_internal.hpp"
#include <iostream>

namespace application {

namespace {

struct PointLightState {
    glm::vec3 position = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
};

struct SpotLightState {
    glm::vec3 position = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 direction = glm::vec3(-1.0f, -1.0f, -1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float inner_angle = 15.0f;
    float outer_angle = 30.0f;
};

struct State {
    int use_perspective = 1;

    glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);
    float camera_fov = 45.0f;

    glm::vec3 light_direction = glm::vec3(0.3f, -1.0f, -0.5f);
    glm::vec3 light_color = glm::vec3(1.0f);
    glm::vec3 ambient_color = glm::vec3(0.1f);

    glm::vec3 material_albedo = glm::vec3(1.0f);
    glm::vec3 material_specular = glm::vec3(1.0f);
    float material_alpha = 32.0f;

    PointLightState point_lights[uniforms::MAX_POINT_LIGHTS];
    int point_light_count = 0;

    SpotLightState spot_lights[uniforms::MAX_SPOT_LIGHTS];
    int spot_light_count = 0;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    bool animate_trajectory = true;
    float trajectory_radius = 1.5f;
    float trajectory_speed = 1.0f;
    bool spin = true;
    float spin_speed = 1.0f;

    int object_count = 4;
} state;

textures::Texture brick_texture;

} // namespace

bool initialize() {
    textures::ImageData image;

    if (!textures::loadImage("textures/brick.jpg", image)) {
        return false;
    }

    auto allocator = graphics::internal::context.allocator;

    if (!textures::createImage(allocator, image, brick_texture)) {
        return false;
    }

    uniforms::setTexture(brick_texture);

    return true;
}

void shutdown() {
	auto& context = graphics::internal::context;
	vkQueueWaitIdle(context.graphics_queue);

    textures::destroyImage(
        context.allocator,
        brick_texture
    );
}

void update([[maybe_unused]] double time) {
    ImGui::Begin("Octahedron Controls");

    ImGui::Text("Projection");
    ImGui::RadioButton("Perspective", &state.use_perspective, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Orthographic", &state.use_perspective, 0);

    ImGui::Separator();

    ImGui::Text("Transform");
    ImGui::SliderFloat3("Position", &state.position.x, -5.0f, 5.0f);
    ImGui::SliderFloat3("Rotation", &state.rotation.x, -180.0f, 180.0f);
    ImGui::SliderFloat3("Scale", &state.scale.x, 0.1f, 3.0f);

    ImGui::Separator();

    ImGui::Text("Animation");
	ImGui::Checkbox("Animate trajectory", &state.animate_trajectory);
	ImGui::SliderFloat("Trajectory radius", &state.trajectory_radius, 0.0f, 5.0f);
	ImGui::SliderFloat("Trajectory speed", &state.trajectory_speed, 0.0f, 5.0f);

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::SliderFloat3("Cam Position", &state.camera_position.x, -10.0f, 10.0f);
    ImGui::SliderFloat3("Cam Target", &state.camera_target.x, -10.0f, 10.0f);
    ImGui::SliderFloat("FOV", &state.camera_fov, 10.0f, 120.0f);
    if (ImGui::Button("Reset Camera")) {
        state.camera_position = glm::vec3(0.0f, 0.0f, 3.0f);
        state.camera_target = glm::vec3(0.0f, 0.0f, 0.0f);
        state.camera_fov = 45.0f;
    }

    ImGui::Separator();
    ImGui::Text("Directional Light");
    ImGui::SliderFloat3("Light Dir", &state.light_direction.x, -1.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &state.light_color.x);
    ImGui::ColorEdit3("Ambient", &state.ambient_color.x);

    ImGui::Separator();
    ImGui::Text("Material");
    ImGui::ColorEdit3("Albedo", &state.material_albedo.x);
    ImGui::ColorEdit3("Specular", &state.material_specular.x);
    ImGui::SliderFloat("Shininess", &state.material_alpha, 1.0f, 256.0f);

    ImGui::Separator();
    ImGui::Text("Point Lights (%d/%d)", state.point_light_count, (int)uniforms::MAX_POINT_LIGHTS);

    if (state.point_light_count < (int)uniforms::MAX_POINT_LIGHTS) {
        if (ImGui::Button("Add Point Light")) {
            int i = state.point_light_count;
            state.point_lights[i].position = glm::vec3(1.0f, 1.0f, 1.0f);
            state.point_lights[i].color = glm::vec3(1.0f);
            state.point_lights[i].intensity = 1.0f;
            state.point_light_count++;
        }
    }

    for (int i = 0; i < state.point_light_count; ++i) {
        ImGui::PushID(i);
        ImGui::Text("Light #%d", i);
        ImGui::SliderFloat3("Pos", &state.point_lights[i].position.x, -10.0f, 10.0f);
        ImGui::ColorEdit3("Color", &state.point_lights[i].color.x);
        ImGui::SliderFloat("Intensity", &state.point_lights[i].intensity, 0.0f, 10.0f);
        if (ImGui::Button("Delete")) {
            for (int j = i; j < state.point_light_count - 1; ++j) {
                state.point_lights[j] = state.point_lights[j + 1];
            }
            state.point_light_count--;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Spot Lights (%d/%d)", state.spot_light_count, (int)uniforms::MAX_SPOT_LIGHTS);

    if (state.spot_light_count < (int)uniforms::MAX_SPOT_LIGHTS) {
        if (ImGui::Button("Add Spot Light")) {
            int i = state.spot_light_count;
            state.spot_lights[i].position = glm::vec3(2.0f, 2.0f, 2.0f);
            state.spot_lights[i].direction = glm::vec3(-1.0f, -1.0f, -1.0f);
            state.spot_lights[i].color = glm::vec3(1.0f);
            state.spot_lights[i].intensity = 2.0f;
            state.spot_lights[i].inner_angle = 15.0f;
            state.spot_lights[i].outer_angle = 30.0f;
            state.spot_light_count++;
        }
    }

    for (int i = 0; i < state.spot_light_count; ++i) {
        ImGui::PushID(1000 + i);
        ImGui::Text("Spot #%d", i);
        ImGui::SliderFloat3("Pos", &state.spot_lights[i].position.x, -10.0f, 10.0f);
        ImGui::SliderFloat3("Dir", &state.spot_lights[i].direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit3("Color", &state.spot_lights[i].color.x);
        ImGui::SliderFloat("Intensity", &state.spot_lights[i].intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Inner", &state.spot_lights[i].inner_angle, 1.0f, 60.0f);
        ImGui::SliderFloat("Outer", &state.spot_lights[i].outer_angle, 1.0f, 90.0f);
        if (ImGui::Button("Delete")) {
            for (int j = i; j < state.spot_light_count - 1; ++j) {
                state.spot_lights[j] = state.spot_lights[j + 1];
            }
            state.spot_light_count--;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

	ImGui::Checkbox("Spin", &state.spin);
	ImGui::SliderFloat("Spin speed", &state.spin_speed, 0.0f, 5.0f);

	ImGui::Separator();

	ImGui::Text("Objects");
	ImGui::SliderInt("Count", &state.object_count, 1, uniforms::MAX_OBJECTS);

    ImGui::End();


    const float t = static_cast<float>(time);

    glm::vec3 cam_target = state.camera_target;
    glm::vec3 cam_pos = state.camera_position;
    if (glm::length(cam_target - cam_pos) < 0.001f) {
        cam_target = cam_pos + glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::mat4 view = glm::lookAt(
        cam_pos,
        cam_target,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    const float aspect = (float)graphics::internal::context.swapchain_extent.width /
                         (float)graphics::internal::context.swapchain_extent.height;

    glm::mat4 proj;
    const float fov_rad = glm::radians(state.camera_fov);
    if (state.use_perspective) {
        proj = glm::perspective(fov_rad, aspect, 0.1f, 100.0f);
    } else {
        const float distance = glm::length(state.camera_target - state.camera_position);
        const float visible_height = 2.0f * distance * glm::tan(fov_rad / 2.0f);
        proj = glm::ortho(-visible_height * aspect / 2.0f,
                           visible_height * aspect / 2.0f,
                          -visible_height / 2.0f,
                           visible_height / 2.0f,
                          -100.0f, 100.0f);
    }
    proj[1][1] *= -1.0f;

    auto* scene = uniforms::getSceneMemory();
    scene->view_projection = proj * view;
    scene->camera_position = glm::vec4(state.camera_position, 1.0f);

    glm::vec3 light_dir_n = state.light_direction;
    if (glm::length(light_dir_n) > 0.0001f) {
        light_dir_n = glm::normalize(light_dir_n);
    } else {
        light_dir_n = glm::vec3(0.0f, -1.0f, 0.0f);
    }
    
    scene->light_direction = glm::vec4(light_dir_n, 0.0f);
    scene->light_color = glm::vec4(state.light_color, 1.0f);
    scene->ambient_color = glm::vec4(state.ambient_color, 1.0f);

    scene->point_light_count = glm::uvec4(static_cast<uint32_t>(state.point_light_count), 0, 0, 0);

    auto* pl_mem = uniforms::getPointLightsMemory();
    for (int i = 0; i < state.point_light_count; ++i) {
        pl_mem[i].position = glm::vec4(state.point_lights[i].position, 1.0f);
        pl_mem[i].color = glm::vec4(state.point_lights[i].color, state.point_lights[i].intensity);
    }

    scene->spot_light_count = glm::uvec4(static_cast<uint32_t>(state.spot_light_count), 0, 0, 0);

    auto* sl_mem = uniforms::getSpotLightsMemory();
    for (int i = 0; i < state.spot_light_count; ++i) {
        sl_mem[i].position = glm::vec4(state.spot_lights[i].position, 1.0f);
        sl_mem[i].direction = glm::vec4(glm::normalize(state.spot_lights[i].direction), 0.0f);
        sl_mem[i].color = glm::vec4(
            state.spot_lights[i].color,
            state.spot_lights[i].intensity
        );
        float inner_cos = glm::cos(glm::radians(state.spot_lights[i].inner_angle));
        float outer_cos = glm::cos(glm::radians(state.spot_lights[i].outer_angle));
        sl_mem[i].params = glm::vec4(inner_cos, outer_cos, 0.0f, 0.0f);
    }

    glm::vec3 trajectory_offset = glm::vec3(0.0f);
    if (state.animate_trajectory) {
        const float traj_angle = t * state.trajectory_speed;
        trajectory_offset.x = state.trajectory_radius * glm::cos(traj_angle);
        trajectory_offset.z = state.trajectory_radius * glm::sin(traj_angle);
    }

    for (int i = 0; i < state.object_count; ++i) {
        const float obj_angle = (float)i / (float)state.object_count * 2.0f * glm::pi<float>();
        const float obj_radius = 2.0f;
        glm::vec3 obj_pos = glm::vec3(
            obj_radius * glm::cos(obj_angle),
            0.0f,
            obj_radius * glm::sin(obj_angle)
        );

        glm::vec3 final_pos = state.position + obj_pos + trajectory_offset;

        const float obj_spin = state.spin ? (t * state.spin_speed + obj_angle) : 0.0f;

        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, final_pos);

        model = glm::rotate(model, glm::radians(state.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(state.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(state.rotation.z), glm::vec3(0, 0, 1));

        model = glm::rotate(model, obj_spin, glm::vec3(0, 1, 0));

        model = glm::scale(model, state.scale * 0.5f);

        auto* mem = uniforms::getModelMemory((uint32_t)i);
        mem->model = model;
        glm::vec3 per_object_tint = glm::vec3(
            0.5f + 0.5f * glm::cos(obj_angle),
            0.5f + 0.5f * glm::sin(obj_angle),
            1.0f
        );
        mem->albedo = glm::vec4(state.material_albedo * per_object_tint, 1.0f);
        mem->specular = glm::vec4(state.material_specular, 1.0f);
        mem->params = glm::vec4(state.material_alpha, 0.0f, 0.0f, 0.0f);
    }
}

void render(const graphics::internal::FrameData& fd) {
    VkCommandBuffer cmd = fd.command_buffer;

    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin);

    const VkClearValue clear_values[] = {
        { .color = { .float32 = { 0.1f, 0.1f, 0.1f, 1.0f } } },
        { .depthStencil = { 1.0f, 0 } },
    };

    const VkRenderPassBeginInfo render_pass = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = graphics::internal::context.render_pass,
        .framebuffer = fd.framebuffer,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = graphics::internal::context.swapchain_extent,
        },
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &render_pass, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = (float)graphics::internal::context.swapchain_extent.width,
        .height = (float)graphics::internal::context.swapchain_extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    const VkRect2D scissor = { .extent = graphics::internal::context.swapchain_extent };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline::getPipeline());

	const VkDeviceSize vertex_offset = 0;
	VkBuffer vb = octahedron::getVertexBuffer();
	vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &vertex_offset);
	vkCmdBindIndexBuffer(cmd, octahedron::getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    const VkDescriptorSet scene_set = uniforms::getSceneSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline::getLayout(), 0, 1, &scene_set, 0, nullptr);

    for (int i = 0; i < state.object_count; ++i) {
        const VkDescriptorSet model_set = uniforms::getModelSet((uint32_t)i);

        const uint32_t dynamic_offset = uniforms::getModelDynamicOffset((uint32_t)i);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline::getLayout(),
                                1, 1, &model_set, 1, &dynamic_offset);

        vkCmdDrawIndexed(cmd, octahedron::index_count, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

} // namespace application