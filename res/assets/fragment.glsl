#version 460 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
in uvec2 fTextureID;

out vec4 FragColor;
  
void main() {
    FragColor = vec4(0.8, 0.4, 0.6, 1.0);
}

