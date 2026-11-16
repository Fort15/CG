#version 450 core

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_world_position;
layout(location = 3) in vec2 in_uv;

layout(set = 1, binding = 1) uniform sampler2D albedo_texture;

layout(location = 0) out vec4 out_color;

struct PointLight {
    vec4 position;
    vec4 color;
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 color;
    vec4 params;
};

layout(std430, set = 0, binding = 2) readonly buffer SpotLights {
    SpotLight spots[];
};

layout(std140, set = 0, binding = 0) uniform SceneUniforms {
    mat4 view_projection;
    vec4 camera_position;
    vec4 light_direction;
    vec4 light_color;
    vec4 ambient_color;
    uvec4 point_light_count;
    uvec4 spot_light_count;
} scene;

layout(std430, set = 0, binding = 1) readonly buffer PointLights {
    PointLight lights[];
};

layout(std140, set = 1, binding = 0) uniform ModelUniforms {
    mat4 model;
    vec4 albedo;
    vec4 specular;
    vec4 params;
} model_data;

void main() {
    vec3 N = normalize(in_normal);
    vec3 V = normalize(scene.camera_position.xyz - in_world_position);
    vec3 base_color = texture(albedo_texture, in_uv).rgb;

    vec3 result = scene.ambient_color.rgb * base_color;

    {
        vec3 L = normalize(-scene.light_direction.xyz);
        vec3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float spec = NdotL > 0.0 ? pow(NdotH, model_data.params.x) : 0.0;
        result += scene.light_color.rgb * NdotL * base_color;
        result += scene.light_color.rgb * spec * model_data.specular.rgb;
    }

    uint count = scene.point_light_count.x;
    for (uint i = 0u; i < count; ++i) {
        vec3 to_light = lights[i].position.xyz - in_world_position;
        float distance = length(to_light);
        vec3 L = to_light / max(distance, 0.0001);
        vec3 H = normalize(L + V);

        float attenuation = 1.0 / max(distance * distance, 0.01);
        vec3 lc = lights[i].color.rgb * lights[i].color.a;

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float spec = pow(NdotH, model_data.params.x);

        result += attenuation * lc * (
            NdotL * base_color +
            spec * model_data.specular.rgb
        );
    }

    uint spot_count = scene.spot_light_count.x;
    for (uint i = 0u; i < spot_count; ++i) {
        vec3 to_light = spots[i].position.xyz - in_world_position;
        float distance = length(to_light);
        vec3 L = to_light / max(distance, 0.0001);
        vec3 H = normalize(L + V);

        vec3 to_surface = -L;
        float theta = dot(normalize(spots[i].direction.xyz), to_surface);
        float inner_cos = spots[i].params.x;
        float outer_cos = spots[i].params.y;
        float spot_factor = clamp((theta - outer_cos) / max(inner_cos - outer_cos, 0.0001), 0.0, 1.0);

        float attenuation = 1.0 / max(distance * distance, 0.01);
        vec3 lc = spots[i].color.rgb * spots[i].color.a;

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float spec = pow(NdotH, model_data.params.x);

        result += spot_factor * attenuation * lc * (
            NdotL * base_color +
            spec * model_data.specular.rgb
        );
    }

    out_color = vec4(result, 1.0);
}