#ifndef LAB4_CHARACTER_H
#define LAB4_CHARACTER_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <vector>
#include <map>
#include <string>

struct MyBot {
    void initialize();
    void update(float time);
    void render(glm::mat4 cameraMatrix);
    void cleanup();

private:
    GLuint mvpMatrixID;
    GLuint jointMatricesID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    GLuint programID;

    tinygltf::Model model;

    struct PrimitiveObject {
        GLuint vao;
        std::map<int, GLuint> vbos;
    };
    std::vector<PrimitiveObject> primitiveObjects;

    struct SkinObject {
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<glm::mat4> globalJointTransforms;
        std::vector<glm::mat4> jointMatrices;
    };
    std::vector<SkinObject> skinObjects;

    struct AnimationObject {
        struct SamplerObject {
            std::vector<float> input;
            std::vector<glm::vec4> output;
            int interpolation;
        };

        std::vector<SamplerObject> samplers;
    };
    std::vector<AnimationObject> animationObjects;

    glm::mat4 getNodeTransform(const tinygltf::Node &node);
    void computeLocalNodeTransform(const tinygltf::Model &model, int nodeIndex, std::vector<glm::mat4> &localTransforms);
    void computeGlobalNodeTransform(const tinygltf::Model &model, const std::vector<glm::mat4> &localTransforms, int nodeIndex, const glm::mat4 &parentTransform, std::vector<glm::mat4> &globalTransforms);
    std::vector<SkinObject> prepareSkinning(const tinygltf::Model &model);
    std::vector<AnimationObject> prepareAnimation(const tinygltf::Model &model);

    int findKeyframeIndex(const std::vector<float> &times, float animationTime);

    void updateAnimation(const tinygltf::Model &model, const tinygltf::Animation &anim, const AnimationObject &animationObject, float time, std::vector<glm::mat4> &nodeTransforms);
    void updateSkinning(const std::vector<glm::mat4> &globalTransforms);
    bool loadModel(tinygltf::Model &model, const char *filename);
    std::vector<PrimitiveObject> bindModel(tinygltf::Model &model);
    void bindMesh(std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Mesh &mesh);
    void bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Node &node);
    void drawModel(const std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model);
    void drawMesh(const std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Mesh &mesh);
    void drawModelNodes(const std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model, tinygltf::Node &node);
};

#endif
