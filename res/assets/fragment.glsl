#version 460 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
flat in uint fInstance;
flat in uint fFlags;
flat in vec4 fColour;

layout(binding = 3, std430) readonly buffer ssbo2 {
    sampler2D sprite_textures[];
};

out vec4 FragColor;
  
void main() {
    vec2 uvs = vec2(fUVs.x, 1.0 - fUVs.y);
    vec4 sampled = texture(sprite_textures[fInstance], uvs);

    vec4 tint = mix(vec4(1.0), fColour, fFlags & 2);
    vec4 color = mix(sampled, vec4(tint.rgb, sampled.r), fFlags & 1);

    FragColor = color;
}

