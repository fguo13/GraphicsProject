#version 330 core

in vec2 uv;

out vec4 FragColor;

uniform sampler2D textureSampler;

void main() {
    // Sample the texture using UV coordinates
    FragColor = texture(textureSampler, uv);
}
