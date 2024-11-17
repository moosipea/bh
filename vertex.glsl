#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 transforms[256];

void main() {
    vec4 pos = transforms[gl_InstanceID] * vec4(aPos, 1.0);
    gl_Position = pos; 
} 

