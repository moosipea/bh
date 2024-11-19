#version 460 core

struct sprite {
    sampler2D texture_handle;
    mat4 transform;   
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUVs;

uniform mat4 projection_matrix;

layout(binding = 2, std430) readonly buffer ssbo1 {
    struct sprite sprite_data[];
};

out vec2 fUVs;
out uvec2 fTextureID;

void main() {
    vec4 pos = projection_matrix * model_matrices[gl_InstanceID].transform * vec4(aPos, 1.0);
    gl_Position = pos;

    fUVs = aUVs;
    fTextureID = sprite_data[gl_InstanceID].texture_handle;
}

