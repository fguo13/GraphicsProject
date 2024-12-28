#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include <iostream>
#include <iomanip>

#include "skybox.h"
#include "lab4_character.h"

glm::vec3 eye_center(0.0f, 100.0f, 800.0f);
glm::vec3 lookat(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
float viewDistance = 600.0f, viewAzimuth = 0.0f, viewPolar = -1.5f;

GLFWwindow *window;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    static float viewPitch = 0.0f;
    static float viewAzimuth = 0.0f;
    const float maxPitch = glm::radians(89.0f); // Prevent looking too far up/down
    const float moveSpeed = 5.0f;              // Speed of movement
    const float minHeight = -5.0f;
    const float maxHeight = 300.0f;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        eye_center = glm::vec3(0.0f, 100.0f, 800.0f);
        viewPitch = 0.0f;
        viewAzimuth = 0.0f;
    }

    if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch += glm::radians(2.0f);
        viewPitch = glm::clamp(viewPitch, -maxPitch, maxPitch);
    }

    if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch -= glm::radians(2.0f);
        viewPitch = glm::clamp(viewPitch, -maxPitch, maxPitch);
    }

    if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth -= glm::radians(2.0f);
    }

    if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth += glm::radians(2.0f);
    }

    if (eye_center.y < minHeight) {
        eye_center.y = minHeight;
    }
    if (eye_center.y > maxHeight) {
        eye_center.y = maxHeight;
    }
    glm::vec3 forward(
        cos(viewPitch) * cos(viewAzimuth),
        sin(viewPitch),
        cos(viewPitch) * sin(viewAzimuth));
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        eye_center += forward * moveSpeed;
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        eye_center -= forward * moveSpeed;
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        eye_center -= right * moveSpeed;
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        eye_center += right * moveSpeed;
    }

    lookat = eye_center + forward;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}



int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

    window = glfwCreateWindow(1024, 768, "Project", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD.\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    initSkybox();
    initPlane();

    float FoV = 80.0f;
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 1024.0f / 768.0f, 0.1f, 1000.0f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
        renderSkybox(viewMatrix, projectionMatrix, eye_center);
        renderPlane(viewMatrix, projectionMatrix);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


