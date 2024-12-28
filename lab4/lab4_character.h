#ifndef LAB4_CHARACTER_H
#define LAB4_CHARACTER_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_gltf.h>
#include <vector>
#include <map>
#include <iostream>

struct MyBot {
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

    void initialize();
    void update(float time);
    void render(glm::mat4 vpMatrix);
    void cleanup();

    // Helper functions (declarations only)
    bool loadModel(tinygltf::Model &model, const char *filename);
    std::vector<PrimitiveObject> bindModel(tinygltf::Model &model);
    void drawModel(const std::vector<PrimitiveObject> &primitiveObjects, tinygltf::Model &model);
};

#endif // LAB4_CHARACTER_H
