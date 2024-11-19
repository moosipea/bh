#version 400 core

#extension GL_ARB_bindless_texture : require

in vec2 fUVs;
in int fInstance;

out vec4 FragColor;
  
void main() {
    FragColor = vec4(0.8, 0.4, 0.6, 1.0);
}

