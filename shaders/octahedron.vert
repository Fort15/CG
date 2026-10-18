#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_world_position;

layout(std140, set = 0, binding = 0) uniform SceneUniforms {
    mat4 view_projection;
    vec4 camera_position;
    vec4 light_direction;
    vec4 light_color;
    vec4 ambient_color;
} scene;

layout(std140, set = 1, binding = 0) uniform ModelUniforms {
    mat4 model;
    vec4 albedo;
    vec4 specular;
    float alpha;
} model_data;

void main() {
    vec4 world_pos = model_data.model * vec4(in_position, 1.0);
    gl_Position = scene.view_projection * world_pos;

    out_world_position = world_pos.xyz;
    out_normal = mat3(model_data.model) * in_normal;
    out_color = in_color;
}