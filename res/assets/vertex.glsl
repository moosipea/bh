#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 transforms[128];
uniform mat4 projection_matrix;

void main() {
    vec4 pos = projection_matrix * transforms[gl_InstanceID] * vec4(aPos, 1.0);
    gl_Position = pos;
}

