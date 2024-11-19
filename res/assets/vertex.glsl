#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUVs;

uniform mat4 transforms[128];
uniform mat4 projection_matrix;

out vec2 fUVs;
out int fInstance;

void main() {
    vec4 pos = projection_matrix * transforms[gl_InstanceID] * vec4(aPos, 1.0);
    gl_Position = pos;

    fUVs = aUVs;
    fInstance = gl_InstanceID;
}

