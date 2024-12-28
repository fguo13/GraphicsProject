#include "lab4_character.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <render/shader.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


#define BUFFER_OFFSET(i) ((char *)NULL + (i))

glm::vec3 lightIntensity(5e6f, 5e6f, 5e6f);
glm::vec3 lightPosition(-275.0f, 500.0f, 800.0f);


void MyBot::initialize() {
    if (!loadModel(model, "../lab4/model/bot/bot.gltf")) {
        std::cerr << "Failed to load model.\n";
        return;
    }

    primitiveObjects = bindModel(model);
    skinObjects = prepareSkinning(model);
    animationObjects = prepareAnimation(model);

    programID = LoadShadersFromFile("../lab4/shader/bot.vert", "../lab4/shader/bot.frag");
    if (programID == 0) {
        std::cerr << "Failed to load shaders.\n";
        return;
    }

    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    jointMatricesID = glGetUniformLocation(programID, "jointMatrices");
    lightPositionID = glGetUniformLocation(programID, "lightPosition");
    lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
}

void MyBot::update(float time) {
    if (model.animations.empty()) return;

    const auto &animation = model.animations[0];
    const auto &animationObject = animationObjects[0];
    const auto &skin = model.skins[0];

    std::vector<glm::mat4> nodeTransforms(skin.joints.size(), glm::mat4(1.0f));
    updateAnimation(model, animation, animationObject, time, nodeTransforms);

    int rootNodeIndex = skin.joints[0];
    computeGlobalNodeTransform(model, nodeTransforms, rootNodeIndex, glm::mat4(1.0f), skinObjects[0].globalJointTransforms);
    updateSkinning(skinObjects[0].globalJointTransforms);
}

void MyBot::render(glm::mat4 cameraMatrix) {
    glUseProgram(programID);

    glm::mat4 mvp = cameraMatrix;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(jointMatricesID, 25, GL_FALSE, &skinObjects[0].jointMatrices[0][0][0]);

    glUniform3fv(lightPositionID, 1, &lightPosition[0]);
    glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);

    drawModel(primitiveObjects, model);
}

void MyBot::cleanup() {
    glDeleteProgram(programID);
}

glm::mat4 MyBot::getNodeTransform(const tinygltf::Node &node) {
    glm::mat4 transform(1.0f);
    if (node.matrix.size() == 16) {
        transform = glm::make_mat4(node.matrix.data());
    } else {
        if (!node.translation.empty())
            transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        if (!node.rotation.empty()) {
            glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            transform *= glm::mat4_cast(q);
        }
        if (!node.scale.empty())
            transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
    return transform;
}

void MyBot::computeLocalNodeTransform(const tinygltf::Model &model, int nodeIndex, std::vector<glm::mat4> &localTransforms) {
    const tinygltf::Node &node = model.nodes[nodeIndex];
    localTransforms[nodeIndex] = getNodeTransform(node);
}

void MyBot::computeGlobalNodeTransform(const tinygltf::Model &model, const std::vector<glm::mat4> &localTransforms, int nodeIndex, const glm::mat4 &parentTransform, std::vector<glm::mat4> &globalTransforms) {
    globalTransforms[nodeIndex] = parentTransform * localTransforms[nodeIndex];
    for (int child : model.nodes[nodeIndex].children) {
        computeGlobalNodeTransform(model, localTransforms, child, globalTransforms[nodeIndex], globalTransforms);
    }
}

std::vector<MyBot::SkinObject> MyBot::prepareSkinning(const tinygltf::Model &model) {
    std::vector<SkinObject> skinObjects;

    for (const auto &skin : model.skins) {
        SkinObject skinObject;
        const auto &accessor = model.accessors[skin.inverseBindMatrices];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        const float *ptr = reinterpret_cast<const float *>(
            buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);

        skinObject.inverseBindMatrices.resize(accessor.count);
        for (size_t j = 0; j < accessor.count; ++j) {
            float m[16];
            memcpy(m, ptr + j * 16, 16 * sizeof(float));
            skinObject.inverseBindMatrices[j] = glm::make_mat4(m);
        }

        skinObject.globalJointTransforms.resize(skin.joints.size());
        skinObject.jointMatrices.resize(skin.joints.size());

        skinObjects.push_back(skinObject);
    }
    return skinObjects;
}

std::vector<MyBot::AnimationObject> MyBot::prepareAnimation(const tinygltf::Model &model) {
    std::vector<AnimationObject> animationObjects;

    for (const auto &anim : model.animations) {
        AnimationObject animationObject;

        for (const auto &sampler : anim.samplers) {
            AnimationObject::SamplerObject samplerObject;


            const tinygltf::Accessor &inputAccessor = model.accessors[sampler.input];
            const tinygltf::BufferView &inputBufferView = model.bufferViews[inputAccessor.bufferView];
            const tinygltf::Buffer &inputBuffer = model.buffers[inputBufferView.buffer];

            assert(inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(inputAccessor.type == TINYGLTF_TYPE_SCALAR);

            // Input (time) values
            samplerObject.input.resize(inputAccessor.count);

            const unsigned char *inputPtr = &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset];
            int stride = inputAccessor.ByteStride(inputBufferView);
            for (size_t i = 0; i < inputAccessor.count; ++i) {
                samplerObject.input[i] = *reinterpret_cast<const float *>(inputPtr + i * stride);
            }

            const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
            const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
            const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

            assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

            const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
            int outputStride = outputAccessor.ByteStride(outputBufferView);

            // Output values
            samplerObject.output.resize(outputAccessor.count);

            for (size_t i = 0; i < outputAccessor.count; ++i) {
                if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
                    memcpy(&samplerObject.output[i], outputPtr + i * 3 * sizeof(float), 3 * sizeof(float));
                } else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
                    memcpy(&samplerObject.output[i], outputPtr + i * 4 * sizeof(float), 4 * sizeof(float));
                } else {
                    std::cerr << "Unsupported accessor type.\n";
                }
            }

            animationObject.samplers.push_back(samplerObject);
        }

        animationObjects.push_back(animationObject);
    }

    return animationObjects;
}

int MyBot::findKeyframeIndex(const std::vector<float> &times, float animationTime) {
    int left = 0;
    int right = static_cast<int>(times.size()) - 1;

    while (left <= right) {
        int mid = (left + right) / 2;

        if (mid + 1 < static_cast<int>(times.size()) &&
            times[mid] <= animationTime &&
            animationTime < times[mid + 1]) {
            return mid;
            } else if (times[mid] > animationTime) {
                right = mid - 1;
            } else { // animationTime >= times[mid + 1]
                left = mid + 1;
            }
    }

    // If no valid keyframe is found, return the second-to-last keyframe
    return static_cast<int>(times.size()) - 2;
}


void MyBot::updateAnimation(const tinygltf::Model &model,
                            const tinygltf::Animation &anim,
                            const AnimationObject &animationObject,
                            float time,
                            std::vector<glm::mat4> &nodeTransforms) {
    for (const auto &channel : anim.channels) {
        int targetNodeIndex = channel.target_node;
        const auto &sampler = anim.samplers[channel.sampler];

        const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
        const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
        const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

        const std::vector<float> &times = animationObject.samplers[channel.sampler].input;
        float animationTime = fmod(time, times.back());

        // Find keyframe index
        int keyframeIndex = findKeyframeIndex(times, animationTime);
        float t = (animationTime - times[keyframeIndex]) / (times[keyframeIndex + 1] - times[keyframeIndex]);

        const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];

        // Interpolate and apply animation
        if (channel.target_path == "translation") {
            glm::vec3 translation0, translation1;
            memcpy(&translation0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));

            glm::vec3 translation = translation0;
            nodeTransforms[targetNodeIndex] = glm::translate(nodeTransforms[targetNodeIndex], translation);
        } else if (channel.target_path == "rotation") {
            glm::quat rotation0, rotation1;
            memcpy(&rotation0, outputPtr + keyframeIndex * 4 * sizeof(float), 4 * sizeof(float));

            glm::quat rotation = rotation0;
            nodeTransforms[targetNodeIndex] *= glm::mat4_cast(rotation);
        } else if (channel.target_path == "scale") {
            glm::vec3 scale0, scale1;
            memcpy(&scale0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));

            glm::vec3 scale = scale0;
            nodeTransforms[targetNodeIndex] = glm::scale(nodeTransforms[targetNodeIndex], scale);
        }
    }
}

void MyBot::updateSkinning(const std::vector<glm::mat4> &globalTransforms) {
    int globalIndexToJointIndex[] = {24, 3, 2, 1, 0, 7, 6, 5, 4, 23, 22, 9, 8, 15, 14, 13, 12, 10, 11, 21, 20, 19, 18, 16, 17};
    for (size_t i = 0; i < skinObjects.size(); ++i) {
        SkinObject &skin = skinObjects[i];
        for (size_t j = 0; j < skin.globalJointTransforms.size(); ++j) {
            skin.jointMatrices[j] = globalTransforms[globalIndexToJointIndex[j]] * skin.inverseBindMatrices[j];
        }
    }
}


bool MyBot::loadModel(tinygltf::Model &model, const char *filename) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << "ERR: " << err << std::endl;
    }
    if (!res) {
        std::cout << "Failed to load glTF: " << filename << std::endl;
    } else {
        std::cout << "Loaded glTF: " << filename << std::endl;
    }
    return res;
}

std::vector<MyBot::PrimitiveObject> MyBot::bindModel(tinygltf::Model &model) {
    std::vector<PrimitiveObject> primitiveObjects;

    const auto &scene = model.scenes[model.defaultScene];
    for (int nodeIndex : scene.nodes) {
        bindModelNodes(primitiveObjects, model, model.nodes[nodeIndex]);
    }

    return primitiveObjects;
}

void MyBot::bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Node &node) {
    if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
        bindMesh(primitiveObjects, model, model.meshes[node.mesh]);
    }

    for (int child : node.children) {
        bindModelNodes(primitiveObjects, model, model.nodes[child]);
    }
}

void MyBot::bindMesh(std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Mesh &mesh) {
    for (const auto &primitive : mesh.primitives) {
        PrimitiveObject primitiveObject;
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        for (const auto &[attribName, accessorIndex] : primitive.attributes) {
            const auto &accessor = model.accessors[accessorIndex];
            const auto &bufferView = model.bufferViews[accessor.bufferView];
            const auto &buffer = model.buffers[bufferView.buffer];

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, &buffer.data[bufferView.byteOffset], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0); // Adjust based on attribute type
            glVertexAttribPointer(0, accessor.type, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, accessor.ByteStride(bufferView), BUFFER_OFFSET(accessor.byteOffset));

            primitiveObject.vbos[accessorIndex] = vbo;
        }

        primitiveObject.vao = vao;
        primitiveObjects.push_back(primitiveObject);
    }
}

void MyBot::drawModel(const std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model) {
    for (const auto &primitiveObject : primitiveObjects) {
        glBindVertexArray(primitiveObject.vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}
