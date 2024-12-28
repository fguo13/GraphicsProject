#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <iostream>

// Skybox global variables
extern GLuint skyboxVAO, skyboxVBO, skyboxEBO;
extern GLuint skyboxTexture;
extern GLuint programID;

// Plane global variables
extern GLuint planeVAO, planeVBO, planeEBO;
extern GLuint planeTexture;
extern GLuint planeProgramID;

// Functions
void initSkybox();
void renderSkybox(glm::mat4 view, glm::mat4 projection, glm::vec3 cameraPosition);

void initPlane();
void renderPlane(glm::mat4 view, glm::mat4 projection);

#endif // SKYBOX_H
