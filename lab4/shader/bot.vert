#version 330 core

layout(location = 0) in vec3 vertexPosition;  // Vertex position
layout(location = 1) in vec3 vertexNormal;    // Vertex normal
layout(location = 3) in vec4 jointIndices;   // Indices of joints influencing the vertex
layout(location = 4) in vec4 jointWeights;    // Weights of the joints

uniform mat4 jointMatrices[100];  // Array of joint matrices (adjust size as needed)
uniform mat4 MVP;                 // Model-View-Projection matrix

out vec3 fragNormal;
out vec3 worldPosition;
out vec3 worldNormal;

void main() {
    mat4 skinMatrix = mat4(0.0);

    // Compute the skinning matrix
    for (int i = 0; i < 4; i++) {
        skinMatrix += jointWeights[i] * jointMatrices[int(jointIndices[i])];
    }

    // Apply skinning transformation
    vec4 skinnedPosition = skinMatrix * vec4(vertexPosition, 1.0);
    gl_Position = MVP * skinnedPosition;

    // Transform normal
    fragNormal = mat3(skinMatrix) * vertexNormal;

    worldPosition = vertexPosition;
    worldNormal = vertexNormal;
}