#version 460 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
flat in int fInstance;

layout(binding = 3, std430) readonly buffer ssbo2 {
    sampler2D sprite_textures[];
};

out vec4 FragColor;
  
void main() {
    FragColor = vec4(texture(sprite_textures[fInstance], fUVs).rgb, 1.0);
}

