#version 460 core

in vec2 fUVs;

out vec4 FragColor;

uniform sampler2D screen_texture;

// Adapted from: https://www.geeks3d.com/20140213/glsl-shader-library-fish-eye-and-dome-and-barrel-distortion-post-processing-filters/2/
vec2 barrel(vec2 uvs) {
    vec2 xy = 2.0 * uvs.xy - 1.0;
    float theta = atan(xy.x, xy.y);
    float r = pow(length(xy), 1.2);
    return 0.5 * (r * vec2(sin(theta), cos(theta)) + 1);
}

void main() {
    vec2 uvs = vec2(fUVs.x, 1.0 - fUVs.y);

    vec3 sampled = texture(screen_texture, barrel(uvs)).rgb; 
    FragColor = vec4(sampled, 1.0);
}
