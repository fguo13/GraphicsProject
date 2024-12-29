#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 3) in vec4 jointIndices;
layout(location = 4) in vec4 jointWeights;

uniform mat4 jointMatrices[100];
uniform mat4 MVP;

out vec3 fragNormal;
out vec3 worldPosition;
out vec3 worldNormal;

uniform FirstHalf { mat4[1000] firstHalfOfJoints; };

uniform SecondHalf { mat4[112] secondHalfOfJoints; };

mat4 indexJoint(int index) {
    if (index < 1000)
        return firstHalfOfJoints[index];
    else
        return secondHalfOfJoints[index - 1000];
}

void main() {
    mat4 skinMatrix = mat4(0.0);

    // Compute the skinning matrix
    for (int i = 0; i < 4; i++) {
        skinMatrix += jointWeights[i] * indexJoint(int(jointIndices[i]));
    }

    // Apply skinning transformation
    vec4 skinnedPosition = skinMatrix * vec4(vertexPosition, 1.0);
    gl_Position = MVP * skinnedPosition;

    // Transform normal
    fragNormal = mat3(skinMatrix) * vertexNormal;

    worldPosition = vertexPosition;
    worldNormal = vertexNormal;
}