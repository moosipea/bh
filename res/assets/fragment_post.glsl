#version 460 core

in vec2 fUVs;

out vec4 FragColor;

uniform sampler2D screen_texture;

void main() {
    vec2 uvs = vec2(fUVs.x, 1.0 - fUVs.y);
    vec3 sampled = texture(screen_texture, uvs).rgb; 
    FragColor = vec4(sampled, 1.0);
}
