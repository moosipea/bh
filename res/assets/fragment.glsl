#version 460 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
flat in int fInstance;

layout(binding = 3, std430) readonly buffer ssbo2 {
    sampler2D sprite_textures[];
};

out vec4 FragColor;
  
void main() {
    vec2 uvs = vec2(fUVs.x, 1.0 - fUVs.y);
    FragColor = vec4(texture(sprite_textures[fInstance], uvs).rgb, 1.0);
}

