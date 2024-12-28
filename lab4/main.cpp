#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include <iostream>
#include <iomanip>

// Include headers for skybox and character functionalities
#include "skybox.h"
#include "lab4_character.h"

// Global variables for camera and projection
glm::vec3 eye_center(0.0f, 100.0f, 800.0f);
glm::vec3 lookat(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
float viewDistance = 600.0f, viewAzimuth = 0.0f, viewPolar = -1.5f;

GLFWwindow *window;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    static float viewPitch = 0.0f;
    static float viewAzimuth = 0.0f;
    const float maxPitch = glm::radians(89.0f); // Stops looking too far down/up
    const float minHeight = -5.0f;
    const float maxHeight = 300.0f;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        eye_center = glm::vec3(-278.0f, 273.0f, 800.0f);
        viewPitch = 0.0f;
    }

    if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch += glm::radians(2.0f);
        if (viewPitch < -maxPitch) {
            viewPitch = -maxPitch;
        }
    }

    if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch -= glm::radians(2.0f);
        if (viewPitch > maxPitch) {
            viewPitch = maxPitch;
        }
    }

    if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth -= glm::radians(2.0f);
    }

    if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth += glm::radians(2.0f);
    }

    lookat = eye_center + glm::vec3(
        cos(viewPitch) * cos(viewAzimuth),
        sin(viewPitch),
        cos(viewPitch) * sin(viewAzimuth)
    );

    if (eye_center.y < minHeight) {
        eye_center.y = minHeight;
    }
    if (eye_center.y > maxHeight) {
        eye_center.y = maxHeight;
    }

    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        eye_center += forward * 5.0f;
        lookat += forward * 5.0f;
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        glm::vec3 backward = glm::normalize(eye_center - lookat);
        eye_center += backward * 5.0f;
        lookat += backward * 5.0f;
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        glm::vec3 left = glm::normalize(glm::cross(lookat - eye_center, glm::vec3(0.0f, 1.0f, 0.0f))); // Fix the order for left
        eye_center -= left * 5.0f;
        lookat -= left * 5.0f;
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        glm::vec3 right = glm::normalize(glm::cross(lookat - eye_center, glm::vec3(0.0f, 1.0f, 0.0f)));
        eye_center += right * 5.0f;
        lookat += right * 5.0f;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

    // Create window and OpenGL context
    window = glfwCreateWindow(1024, 768, "Lab 4 - Combined Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Load OpenGL functions
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD.\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Initialize skybox and plane
    initSkybox();
    initPlane();

    // Initialize character
    MyBot bot;
    bot.initialize();

    // Projection matrix
    float FoV = 80.0f;
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 1024.0f / 768.0f, 0.1f, 1000.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update camera position
        eye_center.y = viewDistance * cos(viewPolar);
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);

        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);

        // Render skybox
        renderSkybox(viewMatrix, projectionMatrix, eye_center);

        // Render plane
        renderPlane(viewMatrix, projectionMatrix);

        // Render character
        glm::mat4 vpMatrix = projectionMatrix * viewMatrix;
        bot.render(vpMatrix);

        // Swap buffers and poll for input events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    bot.cleanup();
    glfwTerminate();
    return 0;
}


