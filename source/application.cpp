#include "application.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include "octahedron.hpp"
#include "pipeline.hpp"
#include "uniforms.hpp"

namespace application {

namespace {

struct State {
    int use_perspective = 1;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 color = glm::vec3(1.0f);

    bool animate_trajectory = true;
    float trajectory_radius = 1.5f;
    float trajectory_speed = 1.0f;
    bool spin = true;
    float spin_speed = 1.0f;

    int object_count = 4;
} state;

} // namespace

bool initialize() {
	return true;
}

void shutdown() {
	auto& context = graphics::internal::context;
	vkQueueWaitIdle(context.graphics_queue);
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

    ImGui::Text("Color");
    ImGui::ColorEdit3("Shape color", &state.color.x);

    ImGui::Separator();

    ImGui::Text("Animation");
	ImGui::Checkbox("Animate trajectory", &state.animate_trajectory);
	ImGui::SliderFloat("Trajectory radius", &state.trajectory_radius, 0.0f, 5.0f);
	ImGui::SliderFloat("Trajectory speed", &state.trajectory_speed, 0.0f, 5.0f);

	ImGui::Checkbox("Spin", &state.spin);
	ImGui::SliderFloat("Spin speed", &state.spin_speed, 0.0f, 5.0f);

	ImGui::Separator();

	ImGui::Text("Objects");
	ImGui::SliderInt("Count", &state.object_count, 1, uniforms::MAX_OBJECTS);

    ImGui::End();


    const float t = static_cast<float>(time);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    const float aspect = (float)graphics::internal::context.swapchain_extent.width /
                         (float)graphics::internal::context.swapchain_extent.height;

    glm::mat4 proj;
    if (state.use_perspective) {
        proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    } else {
        const float distance = 3.0f;
        const float fov = glm::radians(45.0f);
        const float visible_height = 2.0f * distance * glm::tan(fov / 2.0f);
        proj = glm::ortho(-visible_height * aspect / 2.0f,
                           visible_height * aspect / 2.0f,
                          -visible_height / 2.0f,
                           visible_height / 2.0f,
                          -100.0f, 100.0f);
    }
    proj[1][1] *= -1.0f;

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

        glm::mat4 mvp = proj * view * model;

        glm::vec4 color = glm::vec4(
            state.color.r * (0.5f + 0.5f * glm::cos(obj_angle)),
            state.color.g * (0.5f + 0.5f * glm::sin(obj_angle)),
            state.color.b,
            1.0f
        );

        auto* mem = uniforms::getMappedMemory((uint32_t)i);
        mem->mvp = mvp;
        mem->color = color;
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

	for (int i = 0; i < state.object_count; ++i) {
		const VkDescriptorSet set = uniforms::getDescriptorSet((uint32_t)i);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
								pipeline::getLayout(), 0, 1, &set, 0, nullptr);
		vkCmdDrawIndexed(cmd, octahedron::index_count, 1, 0, 0, 0);
	}

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

} // namespace application