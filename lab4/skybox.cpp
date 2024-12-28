#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

GLuint skyboxVAO, skyboxVBO, skyboxEBO;
GLuint planeVAO, planeVBO, planeEBO;
GLuint skyboxTexture;
GLuint planeTexture;
GLuint programID;
GLuint planeProgramID;
GLuint uvBufferID;

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.f;
static float viewPolar = -1.5f;
static float viewDistance = 600.0f;

GLuint LoadTextureTileBox(const char* textureFilePath) {
    int width, height, channels;
    unsigned char* img = stbi_load(textureFilePath, &width, &height, &channels, 0);
    if (!img) {
        std::cerr << "Failed to load texture: " << textureFilePath << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture wrapping and filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload the texture to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, (channels == 4 ? GL_RGBA : GL_RGB), width, height, 0,
                 (channels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img); // Free the image memory after uploading
    return texture;
}

void initPlane() {
    GLfloat planeVertices[] = {
        -1.0f, 0.0f, -1.0f,   0.0f, 0.0f,  // Bottom left
         1.0f, 0.0f, -1.0f,   10.0f, 0.0f, // Bottom right
         1.0f, 0.0f,  1.0f,   10.0f, 10.0f,// Top right
        -1.0f, 0.0f,  1.0f,   0.0f, 10.0f  // Top left
    };


    GLuint planeIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    planeTexture = LoadTextureTileBox("../lab4/shader/grasstexture.jpg");

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);
    glGenBuffers(1, &uvBufferID);

    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Load shaders
    planeProgramID = LoadShadersFromFile("../lab4/shader/plane.vert", "../lab4/shader/plane.frag");
        if (planeProgramID == 0) {
            std::cerr << "Failed to load plane shaders." << std::endl;
        }

    glBindVertexArray(0);
}

static GLuint LoadSkyBoxTexture(const char* texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    if (!img) {
        std::cerr << "Failed to load texture " << texture_file_path << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);

    return texture;
}

GLfloat skyboxVertices[] = {	// Vertex definition for a canonical box
    // Front face
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,

    // Back face
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,

    // Left face
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    // Right face
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,

    // Top face
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    // Bottom face
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
};

GLfloat offset = 0.004f;
GLfloat skyboxUVs[] = {

    // Front
    0.25f + offset, 0.667f - offset,
    0.0f + offset, 0.667f - offset,
    0.0f + offset, 0.33f + offset,
    0.25f + offset, 0.33f + offset,

    // Back
    0.75f - offset, 0.667f - offset,
    0.5f - offset, 0.667f - offset,
    0.5f - offset, 0.33f + offset,
    0.75f - offset, 0.33f + offset,

    // Left
    0.5f - offset, 0.667f - offset,
    0.25f + offset, 0.667f - offset,
    0.25f + offset, 0.33f + offset,
    0.5f - offset, 0.33f + offset,

    // Right
    1.0f - offset, 0.667f - offset,
    0.75f - offset, 0.667f - offset,
    0.75f - offset, 0.33f + offset,
    1.0f - offset, 0.33f + offset,

    // Top
    0.25f + offset, 0.33f + offset,
    0.25f + offset, 0.0f + offset,
    0.5f - offset, 0.0f + offset,
    0.5f - offset, 0.33f + offset,

    // Bottom
    0.5f - offset, 0.66667f - offset,
    0.5f - offset, 1.0f - offset,
    0.25f + offset, 1.0f - offset,
    0.25f + offset, 0.66667f - offset,
};






// Indices for drawing the cube faces
GLuint skyboxIndices[] = {
    0, 1, 2, 0, 2, 3,   4, 5, 6, 4, 6, 7,
    8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23
};

void initSkybox() {
    skyboxTexture = LoadSkyBoxTexture("../lab4/shader/night2.png");

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices) + sizeof(skyboxUVs), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(skyboxVertices), skyboxVertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), sizeof(skyboxUVs), skyboxUVs);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(skyboxVertices)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Load shaders
    programID = LoadShadersFromFile("../lab4/shader/skybox.vert", "../lab4/shader/skybox.frag");
    if (programID == 0) {
        std::cerr << "Failed to load shaders." << std::endl;
    }
}

void renderSkybox(glm::mat4 view, glm::mat4 projection) {
    // Set depth function to allow skybox to be rendered in the background
    glDepthFunc(GL_LEQUAL);

    // Use the skybox shader program
    glUseProgram(programID);


    // Remove translation from the view matrix
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));


    // Compute the MVP matrix with a large scale
    glm::mat4 mvp = projection * viewNoTranslation * glm::scale(glm::mat4(1.0f), glm::vec3(500.0f));

    // Pass the MVP matrix to the shader
    GLuint mvpLoc = glGetUniformLocation(programID, "MVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);

    // Bind the texture
    GLuint textureSamplerLoc = glGetUniformLocation(programID, "textureSampler");
    glUniform1i(textureSamplerLoc, 0); // GL_TEXTURE0

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyboxTexture);

    // Render the skybox
    glBindVertexArray(skyboxVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Reset depth function
    glDepthFunc(GL_LESS);
}

void renderPlane(glm::mat4 view, glm::mat4 projection) {
    glUseProgram(planeProgramID);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -10.0f, 0.0f)); // Lower plane
    model = glm::scale(model, glm::vec3(5000.0f, 1.0f, 5000.0f)); // Make infinite

    GLuint modelLoc = glGetUniformLocation(planeProgramID, "model");
    GLuint viewLoc = glGetUniformLocation(planeProgramID, "view");
    GLuint projLoc = glGetUniformLocation(planeProgramID, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint textureSamplerID = glGetUniformLocation(programID, "textureSampler");
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);
    glUniform1i(textureSamplerID, 0);

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}



// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{

    float floorHeight = -7.0f; // Minimum camera height to prevent passing through the plane

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        viewAzimuth = 0.f;
        viewPolar = 0.f;
        eye_center.y = viewDistance * cos(viewPolar);
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);
        std::cout << "Reset." << std::endl;
    }

    if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPolar -= 0.1f;
        eye_center.y = viewDistance * cos(viewPolar);
    }

    if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPolar += 0.1f;
        eye_center.y = viewDistance * cos(viewPolar);
    }

    if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth -= 0.1f;
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);
    }

    if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth += 0.1f;
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);
    }
    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        eye_center += forward * 5.0f;
        lookat += forward * 5.0f;
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 backward = glm::normalize(eye_center - lookat);
        eye_center += backward * 5.0f;
        lookat += backward * 5.0f;
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 left = glm::normalize(glm::cross(up, lookat - eye_center));
        eye_center -= left * 5.0f;
        lookat -= left * 5.0f;
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 right = glm::normalize(glm::cross(lookat - eye_center, up));
        eye_center += right * 5.0f;
        lookat += right * 5.0f;
    }

    if (eye_center.y < floorHeight) {
        eye_center.y = floorHeight;
    }

    if (lookat.y < floorHeight) {
        lookat.y = floorHeight;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }


}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Skybox Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to open GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, key_callback);



    gladLoadGL(glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    initSkybox();
    eye_center.y = viewDistance * cos(viewPolar);
    eye_center.x = viewDistance * cos(viewAzimuth);
    eye_center.z = viewDistance * sin(viewAzimuth);

    glm::mat4 viewMatrix, projectionMatrix;
    glm::float32 FoV = 80;
    glm::float32 zNear = 0.1f;
    glm::float32 zFar = 1000.0f;
    projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

    initPlane();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 view = glm::lookAt(eye_center, lookat, up);
        renderSkybox(view, projectionMatrix);
        renderPlane(view, projectionMatrix);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


