#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUVs;

uniform mat4 projection_matrix;

struct sprite {
    mat4 transform;
    vec4 colour;
    uint flags;
};

layout(binding = 2, std430) readonly buffer ssbo1 {
    sprite sprite_data[];
};

out vec2 fUVs;
flat out uint fInstance;
flat out uint fFlags;
flat out vec4 fColour;

void main() {
    sprite sp = sprite_data[gl_InstanceID];

    gl_Position = projection_matrix * sp.transform * vec4(aPos, 1.0);

    fUVs = aUVs;
    fInstance = gl_InstanceID;
    fFlags = sp.flags;
    fColour = sp.colour;
}

