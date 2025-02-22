#version 330 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float textureScale; // Repetition factor


out vec2 TexCoords;
out vec4 worldPosition;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    TexCoords = texCoords * textureScale; // Scale UV for texture repetition
    worldPosition = model * vec4(position, 1.0);
}
