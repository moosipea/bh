#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUVs;

uniform mat4 projection_matrix;

layout(binding = 2, std430) readonly buffer ssbo1 {
    mat4 sprite_matrices[];
};

out vec2 fUVs;
flat out int fInstance;

void main() {
    gl_Position = projection_matrix * sprite_matrices[gl_InstanceID] * vec4(aPos, 1.0);
    fUVs = aUVs;
    fInstance = gl_InstanceID;
}

