#version 460 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
in sampler2D fTexture;

out vec4 FragColor;
  
void main() {
    FragColor = texture(fTexture, fUVs);
}

