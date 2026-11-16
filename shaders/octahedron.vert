#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_world_position;
layout(location = 3) out vec2 out_uv;


layout(std140, set = 0, binding = 0) uniform SceneUniforms {
    mat4 view_projection;
    vec4 camera_position;
    vec4 light_direction;
    vec4 light_color;
    vec4 ambient_color;
    uvec4 point_light_count;
    uvec4 spot_light_count;
} scene;

layout(std140, set = 1, binding = 0) uniform ModelUniforms {
    mat4 model;
    vec4 albedo;
    vec4 specular;
    vec4 params;
} model_data;

void main() {
    vec4 world_pos = model_data.model * vec4(in_position, 1.0);
    gl_Position = scene.view_projection * world_pos;

    out_world_position = world_pos.xyz;
    mat3 normal_matrix = transpose(inverse(mat3(model_data.model)));
    out_normal = normalize(normal_matrix * in_normal);
    out_color = in_color;
    out_uv = in_uv;
}