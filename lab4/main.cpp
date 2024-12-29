#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include <iostream>
#include <iomanip>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#undef TINYGLTF_IMPLEMENTATION
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#include <random>

#include "skybox.h"
#include "lab4_character.h"

glm::vec3 eye_center(0.0f, 500.0f, 800.0f);
glm::vec3 lookat(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
float viewDistance = 600.0f, viewPolar = -1.5f;
static float viewPitch = 0.0f;
static float viewAzimuth = 0.0f;
const float maxPitch = glm::radians(89.0f); // Prevent looking too far up/down
const float moveSpeed = 15.0f;              // Speed of movement
const float minHeight = 0.0f;
const float maxHeight = 3000.0f;
static double lastTime = glfwGetTime();
float _time = 0.0f;

GLFWwindow *window;

// Lighting control
const glm::vec3 wave500(0.0f, 255.0f, 146.0f);
const glm::vec3 wave600(255.0f, 190.0f, 0.0f);
const glm::vec3 wave700(205.0f, 0.0f, 0.0f);
static glm::vec3 lightIntensity = 40.0f * (8.0f * wave500 + 15.6f * wave600 + 18.4f * wave700);

// Shadow mapping
static glm::vec3 lightUp(0, 0, 1);
static int shadowMapWidth = 0;
static int shadowMapHeight = 0;
static glm::vec3 mouseOffset = glm::vec3(0.0f);

static float depthFoV = 45.0f;
static float depthNear = 1.0f;
static float depthFar = 1000.0f;

// Helper flag and function to save depth maps for debugging
static bool saveDepth = false;

// This function retrieves and stores the depth map of the default frame buffer
// or a particular frame buffer (indicated by FBO ID) to a PNG image.
static void saveDepthTexture(std::string filename) {
	int width = 1072, height = 768, channels = 3;
	std::vector<float> depth(width * height * 4);
	// glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadBuffer(GL_DEPTH_COMPONENT);
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<unsigned char> img(width * height * channels);
	// glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
	for (int i = 0; i < width * height; ++i)
		img[3 * i] = img[3 * i + 1] = img[3 * i + 2] = depth[i] * 255;
	stbi_write_png(filename.c_str(), width, height, channels, img.data(), width * channels);
}

GLfloat shortBlockVertices[] = {
	// Top face
	-130.0f, 165.0f, -65.0f,   -82.0f, 165.0f, -225.0f,
	-240.0f, 165.0f, -272.0f,  -290.0f, 165.0f, -114.0f,
	// Bottom face
	-130.0f,   0.0f, -65.0f,   -82.0f,   0.0f, -225.0f,
	-240.0f,   0.0f, -272.0f,  -290.0f,   0.0f, -114.0f,
	// Front face
	-130.0f,   0.0f, -65.0f,  -130.0f, 165.0f, -65.0f,
	-290.0f, 165.0f, -114.0f,  -290.0f,   0.0f, -114.0f,
	// Right face
	 -82.0f,   0.0f, -225.0f,   -82.0f, 165.0f, -225.0f,
	-130.0f, 165.0f, -65.0f,  -130.0f,   0.0f, -65.0f,
	// Back face
	-240.0f,   0.0f, -272.0f,  -240.0f, 165.0f, -272.0f,
	 -82.0f, 165.0f, -225.0f,   -82.0f,   0.0f, -225.0f,
	// Left face
	-290.0f,   0.0f, -114.0f,  -290.0f, 165.0f, -114.0f,
	-240.0f, 165.0f, -272.0f,  -240.0f,   0.0f, -272.0f
};

GLfloat shortBlockNormals[] = {
	// Top face
	0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
	// Bottom face
	0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
	// Front face
	0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
	// Right face
	-1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
	// Back face
	0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,
	// Left face
	1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f
};


GLuint shortBlockIndices[] = {
	0, 1, 2,  0, 2, 3,
	4, 5, 6,  4, 6, 7,
	8, 9, 10, 8, 10, 11,
	12, 13, 14, 12, 14, 15,
	16, 17, 18, 16, 18, 19,
	20, 21, 22, 20, 22, 23
};
GLfloat tallBlockVertices[] = {
	// Top face
	-423.0f, 330.0f, -247.0f,  -265.0f, 330.0f, -296.0f,
	-314.0f, 330.0f, -456.0f,  -472.0f, 330.0f, -406.0f,
	// Bottom face
	-423.0f,   0.0f, -247.0f,  -265.0f,   0.0f, -296.0f,
	-314.0f,   0.0f, -456.0f,  -472.0f,   0.0f, -406.0f,
	// Front face
	-423.0f,   0.0f, -247.0f,  -423.0f, 330.0f, -247.0f,
	-472.0f, 330.0f, -406.0f,  -472.0f,   0.0f, -406.0f,
	// Right face
	-472.0f,   0.0f, -406.0f,  -472.0f, 330.0f, -406.0f,
	-314.0f, 330.0f, -456.0f,  -314.0f,   0.0f, -456.0f,
	// Back face
	-314.0f,   0.0f, -456.0f,  -314.0f, 330.0f, -456.0f,
	-265.0f, 330.0f, -296.0f,  -265.0f,   0.0f, -296.0f,
	// Left face
	-265.0f,   0.0f, -296.0f,  -265.0f, 330.0f, -296.0f,
	-423.0f, 330.0f, -247.0f,  -423.0f,   0.0f, -247.0f
};

GLfloat tallBlockNormals[] = {
	// Top face
	0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
	// Bottom face
	0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
	// Front face
	0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
	// Right face
	-1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
	// Back face
	0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,
	// Left face
	1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f
};


GLuint tallBlockIndices[] = {
	0, 1, 2,  0, 2, 3,
	4, 5, 6,  4, 6, 7,
	8, 9, 10, 8, 10, 11,
	12, 13, 14, 12, 14, 15,
	16, 17, 18, 16, 18, 19,
	20, 21, 22, 20, 22, 23
};
GLfloat shortBlockColors[] = {
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};

GLfloat tallBlockColors[] = {
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};
void calculateNormals(GLfloat* vertices, GLfloat* normals, int vertexCount) {
	for (int i = 0; i < vertexCount; i += 12) {
		glm::vec3 p1(vertices[i], vertices[i + 1], vertices[i + 2]);
		glm::vec3 p2(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
		glm::vec3 p3(vertices[i + 6], vertices[i + 7], vertices[i + 8]);

		glm::vec3 v1 = p2 - p1;
		glm::vec3 v2 = p3 - p1;

		glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
		for (int j = 0; j < 4; ++j) {
			normals[i + j * 3] = normal.x;
			normals[i + j * 3 + 1] = normal.y;
			normals[i + j * 3 + 2] = normal.z;
		}
	}
}
void updateShortBlockNormals() {
	calculateNormals(shortBlockVertices, shortBlockNormals, sizeof(shortBlockVertices) / sizeof(shortBlockVertices[0]));
}

void updateTallBlockNormals() {
	calculateNormals(tallBlockVertices, tallBlockNormals, sizeof(tallBlockVertices) / sizeof(tallBlockVertices[0]));
}

unsigned int makeShadowFrameBuffer() {
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
				 1072, 768, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);
	return depthMap;

}




struct CornellBox {

	// Refer to original Cornell Box data
	// from https://www.graphics.cornell.edu/online/box/data.html

	GLfloat vertex_buffer_data[60] = {
		// Floor
		-552.8, 0.0, 0.0,
		0.0, 0.0,   0.0,
		0.0, 0.0, -559.2,
		-549.6, 0.0, -559.2,

		// Ceiling
		-556.0, 548.8, 0.0,
		-556.0, 548.8, -559.2,
		0.0, 548.8, -559.2,
		0.0, 548.8,   0.0,

		// Left wall
		-552.8,   0.0,   0.0,
		-549.6,   0.0, -559.2,
		-556.0, 548.8, -559.2,
		-556.0, 548.8,   0.0,

		// Right wall
		0.0,   0.0, -559.2,
		0.0,   0.0,   0.0,
		0.0, 548.8,   0.0,
		0.0, 548.8, -559.2,

		// Back wall
		-549.6,   0.0, -559.2,
		0.0,   0.0, -559.2,
		0.0, 548.8, -559.2,
		-556.0, 548.8, -559.2
	};


	GLfloat normal_buffer_data[60] = {
		// Floor
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,

		// Ceiling
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,

		// Left wall
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,

		// Right wall
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,

		// Back wall
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	GLfloat color_buffer_data[60] = {
		// Floor
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,

		// Ceiling
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,

		// Left wall
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Right wall
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Back wall
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};

	GLuint index_buffer_data[30] = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,
	};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint normalBufferID;

	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;

	GLuint shortVertexBuffer, shortNormalBuffer, shortIndexBuffer;
	GLuint tallVertexBuffer, tallNormalBuffer, tallIndexBuffer;
	GLuint shortColorBuffer, tallColorBuffer;


	void initialize() {

		updateShortBlockNormals();
		updateTallBlockNormals();
		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the color data
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the vertex normals
		glGenBuffers(1, &normalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);
		glGenBuffers(1, &shortVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, shortVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(shortBlockVertices), shortBlockVertices, GL_STATIC_DRAW);

		glGenBuffers(1, &shortNormalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, shortNormalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(shortBlockNormals), shortBlockNormals, GL_STATIC_DRAW);

		glGenBuffers(1, &shortIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shortIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(shortBlockIndices), shortBlockIndices, GL_STATIC_DRAW);

		// Generate buffers for the tall block
		glGenBuffers(1, &tallVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, tallVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tallBlockVertices), tallBlockVertices, GL_STATIC_DRAW);

		glGenBuffers(1, &tallNormalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, tallNormalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tallBlockNormals), tallBlockNormals, GL_STATIC_DRAW);

		glGenBuffers(1, &tallIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tallIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tallBlockIndices), tallBlockIndices, GL_STATIC_DRAW);
		// Set up color buffer for the short block
		glGenBuffers(1, &shortColorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, shortColorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(shortBlockColors), shortBlockColors, GL_STATIC_DRAW);

		// Set up color buffer for the tall block
		glGenBuffers(1, &tallColorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, tallColorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tallBlockColors), tallBlockColors, GL_STATIC_DRAW);


		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromFile("../lab4/shader/cornell.vert", "../lab4/shader/cornell.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for our "MVP" uniform
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
		glBindVertexArray(0);

	}

	void render(glm::mat4 cameraMatrix) {
    glUseProgram(programID);
		glBindVertexArray(vertexArrayID);

    // Set model-view-projection matrix
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.5f, 0.0f)); // Offset above plane
    glm::mat4 mvp = cameraMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

    // Set light data
    glUniform3fv(lightPositionID, 1, &lightPosition[0]);
    glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);


    // --- Render the main Cornell Box (walls, floor, and ceiling) ---
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    // Draw the Cornell Box (walls, floor, ceiling)
    glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, (void*)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // --- Render the Short Block ---
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, shortVertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, shortColorBuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, shortNormalBuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shortIndexBuffer);
    glDrawElements(GL_TRIANGLES, sizeof(shortBlockIndices) / sizeof(shortBlockIndices[0]), GL_UNSIGNED_INT, (void*)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);

    // --- Render the Tall Block ---
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, tallVertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, tallColorBuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, tallNormalBuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tallIndexBuffer);
    glDrawElements(GL_TRIANGLES, sizeof(tallBlockIndices) / sizeof(tallBlockIndices[0]), GL_UNSIGNED_INT, (void*)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);


    // Unbind the shader program
    glUseProgram(0);
		glBindVertexArray(0);
}


	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteBuffers(1, &normalBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteProgram(programID);
	}
};

void recalculateView() {
	glm::vec3 forward(
		cos(viewPitch) * cos(viewAzimuth),
		sin(viewPitch),
		cos(viewPitch) * sin(viewAzimuth)
	);
	forward = glm::normalize(forward);

	lookat = eye_center + forward;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    const float moveSpeed = 5.0f;
    const float minHeight = -1.0f;
    const float maxHeight = 3000.0f;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        eye_center = glm::vec3(0.0f, 500.0f, 800.0f);
        viewPitch = 0.0f;
        viewAzimuth = 0.0f;
        recalculateView();
    }

    if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch += glm::radians(2.0f);
        viewPitch = glm::clamp(viewPitch, -maxPitch, maxPitch);
        recalculateView();
    }

    if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewPitch -= glm::radians(2.0f);
        viewPitch = glm::clamp(viewPitch, -maxPitch, maxPitch);
        recalculateView();
    }

    if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth -= glm::radians(2.0f);
        recalculateView();
    }

    if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        viewAzimuth += glm::radians(2.0f);
        recalculateView();
    }

    glm::vec3 forward(
        cos(viewPitch) * cos(viewAzimuth),
        sin(viewPitch),
        cos(viewPitch) * sin(viewAzimuth)
    );
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

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

    // Clamp height
    eye_center.y = glm::clamp(eye_center.y, minHeight, maxHeight);

    lookat = eye_center + forward;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}


void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	// Normalize mouse coordinates to [-1, 1]
	float x = (2.0f * xpos) / 1072 - 1.0f;
	float y = 1.0f - (2.0f * ypos) / 768; // Flip Y-axis

	// Scale the normalized coordinates to your scene
	const float sceneScale = 500.0f; // Adjust as needed for sensitivity
	mouseOffset = glm::vec3(x * sceneScale, y * sceneScale, 0.0f);

	// Rotate mouse offset based on camera viewAzimuth
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(viewAzimuth), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(mouseOffset, 1.0f));

	// Compute the final light position
	//lightPosition = eye_center + rotatedOffset;

}




int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "Project", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD.\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

	// Create Cornell Box
	//CornellBox box;
	//box.initialize();

	MyBot jinx;
	jinx.initialize("../lab4/model/jinx/XP_Jinx_Rig.gltf");
	jinx.position.z = 1000.0f;


	MyBot motel;
	motel.initialize("../lab4/model/buildings/motelfix8.gltf");
	motel.position.z = 1000.0f;




    initPlane();

    float FoV = 80.0f;
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 1024.0f / 768.0f, 0.1f, 6000.0f);
	glm::mat4 lightProjectionMatrix = glm::perspective(glm::radians(FoV), 1024.0f / 768.0f, 100.0f, 6000.0f);

	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	unsigned int depthMap = makeShadowFrameBuffer();
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	std::cout<<depthMapFBO<<"is the value of depthMapFBO"<<std::endl;
	std::cout<<depthMap<<"is the value of depthMap"<<std::endl;
	recalculateView();

	std::vector<std::string> modelPaths = {
		"../lab4/model/buildings/building111.gltf",
		"../lab4/model/buildings/building222.gltf",
		"../lab4/model/buildings/building3333.gltf"
	};

	// Vector to store models
	using Model = MyBot;
	std::vector<Model> models;
	float left = -6000.0f;
	float right = -6000.0f;

	// Random number generation setup
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> coordDist(300.0f, 400.0f);
	std::uniform_real_distribution<float> roadOffsetDist(300.0f, 400.0f); // Offset for buildings from the road
	std::uniform_int_distribution<int> pathDist(0, modelPaths.size() - 1);

	for (int i = 0; i < 50; ++i) {
		std::string modelName = modelPaths[i % modelPaths.size()];
		float z = coordDist(gen); // Position along the road (z-axis)
		float offset = roadOffsetDist(gen); // Distance from the road (x-axis)

		// Create buildings on both sides of the road
		MyBot leftBuilding;
		leftBuilding.initialize(modelName.c_str());
		leftBuilding.position = glm::vec3(-offset, 0.0f, left); // Left side of the road
		left += coordDist(gen);
		models.push_back(leftBuilding);

		MyBot rightBuilding;
		rightBuilding.initialize(modelName.c_str());
		rightBuilding.position = glm::vec3(offset, 0.0f, right); // Right side of the road
		right += coordDist(gen);
		models.push_back(rightBuilding);
	}

	initSkybox();

    // Main loop
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		  // Time and frame rate tracking
  static double lastTime = glfwGetTime();
  float time = 0.0f;            // Animation time
  float fTime = 0.0f;            // Time for measuring fps
  unsigned long frames = 0;

		// Rotate mouse offset based on camera viewAzimuth
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -viewAzimuth + glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		mouseOffset.z = 100.0f;
		glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(mouseOffset, 1.0f));

		// Compute the final light position
		lightPosition = eye_center + rotatedOffset;

		// Compute view matrix
		glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vpMatrix = projectionMatrix * viewMatrix;
		glm::mat4 lightViewMatrix = glm::lookAt(lightPosition, lookat, up);
		glm::mat4 lightVpMatrix = lightProjectionMatrix * lightViewMatrix;


		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClearColor(0.2f, 0.2f, 0.25f, 0.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		// Render plane
		renderPlane(lightViewMatrix, lightProjectionMatrix, lightViewMatrix, lightProjectionMatrix);
		for (auto& model : models) {
			model.render(lightVpMatrix);
		}
		jinx.render(lightVpMatrix);
		//box.render(lightVpMatrix);
		//saveDepthTexture("image1.png");
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glActiveTexture(GL_TEXTURE0);

		//viewMatrix = lightViewMatrix;
		//projectionMatrix = lightProjectionMatrix;
		//vpMatrix = lightProjectionMatrix * lightViewMatrix;

		// Render plane
		renderPlane(viewMatrix, projectionMatrix, lightViewMatrix, lightProjectionMatrix);

		// Render Cornell Box
		//box.render(vpMatrix);

		//Render skybox

		renderSkybox(viewMatrix, projectionMatrix, eye_center);

		jinx.render(vpMatrix);
		motel.render(vpMatrix);
		for (auto& model : models) {
			model.render(vpMatrix);
		}
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;
		_time += deltaTime * playbackSpeed;
		jinx.update(_time/2);
		jinx.position.z += 0.8;
		motel.position.y = -2.5f;
		// FPS tracking
		// Count number of frames over a few seconds and take average
		frames++;
		fTime += deltaTime;

			float fps = frames / fTime;
			frames = 0;
			fTime = 0;

			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "Lab 4 | Frames per second (FPS): " << fps;
			glfwSetWindowTitle(window, stream.str().c_str());



		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//box.cleanup();
    glfwTerminate();
    return 0;
}
