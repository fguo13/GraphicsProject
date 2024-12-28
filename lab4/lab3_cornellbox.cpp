#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <render/shader.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void cursor_callback(GLFWwindow *window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 eye_center(-278.0f, 273.0f, 800.0f);
static glm::vec3 lookat(-278.0f, 273.0f, 0.0f);
static glm::vec3 up(0.0f, 1.0f, 0.0f);
static float FoV = 45.0f;
static float zNear = 600.0f;
static float zFar = 1500.0f;

// Lighting control 
const glm::vec3 wave500(0.0f, 255.0f, 146.0f);
const glm::vec3 wave600(255.0f, 190.0f, 0.0f);
const glm::vec3 wave700(205.0f, 0.0f, 0.0f);
static glm::vec3 lightIntensity = 0.0002f * (8.0f * wave500 + 15.6f * wave600 + 18.4f * wave700);
static glm::vec3 lightPosition(-275.0f, 500.0f, -275.0f);

// Shadow mapping
static glm::vec3 lightUp(0, 0, 1);
static int shadowMapWidth = 0;
static int shadowMapHeight = 0;

static float depthFoV = 45.0f;
static float depthNear = 1.0f;
static float depthFar = 1000.0f;

// Helper flag and function to save depth maps for debugging
static bool saveDepth = false;

// This function retrieves and stores the depth map of the default frame buffer 
// or a particular frame buffer (indicated by FBO ID) to a PNG image.
static void saveDepthTexture(GLuint fbo, std::string filename) {
    int width = shadowMapWidth;
    int height = shadowMapHeight;
	if (shadowMapWidth == 0 || shadowMapHeight == 0) {
		width = windowWidth;
		height = windowHeight;
	}
    int channels = 3; 
    
    std::vector<float> depth(width * height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadBuffer(GL_DEPTH_COMPONENT);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<unsigned char> img(width * height * 3);
    for (int i = 0; i < width * height; ++i) img[3*i] = img[3*i+1] = img[3*i+2] = depth[i] * 255;

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

	}

	void render(glm::mat4 cameraMatrix) {
    glUseProgram(programID);

    // Set model-view-projection matrix
    glm::mat4 mvp = cameraMatrix;
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



int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(windowWidth, windowHeight, "Lab 3", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);



	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	glfwSetCursorPosCallback(window, cursor_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	// Prepare shadow map size for shadow mapping. Usually this is the size of the window itself, but on some platforms like Mac this can be 2x the size of the window. Use glfwGetFramebufferSize to get the shadow map size properly.
    glfwGetFramebufferSize(window, &shadowMapWidth, &shadowMapHeight);

	// Background
	glClearColor(0.2f, 0.2f, 0.25f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    // Create the classical Cornell Box
	CornellBox b;
	b.initialize();

	// Camera setup
    glm::mat4 viewMatrix, projectionMatrix;
	projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;

		b.render(vp);

		if (saveDepth) {
            std::string filename = "depth_camera.png";
            saveDepthTexture(0, filename);
            std::cout << "Depth texture saved to " << filename << std::endl;
            saveDepth = false;
        }

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	b.cleanup();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		eye_center = glm::vec3(-278.0f, 273.0f, 800.0f);
		lightPosition = glm::vec3(-275.0f, 500.0f, -275.0f);

	}

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.y += 20.0f;
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.y -= 20.0f;
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x -= 20.0f;
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x += 20.0f;
	}

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.z -= 20.0f;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.z += 20.0f;
	}

	if (key == GLFW_KEY_SPACE && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        saveDepth = true;
    }

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
	if (xpos < 0 || xpos >= windowWidth || ypos < 0 || ypos > windowHeight)
		return;

	// Normalize to [0, 1]
	float x = xpos / windowWidth;
	float y = ypos / windowHeight;

	// To [-1, 1] and flip y up
	x = x * 2.0f - 1.0f;
	y = 1.0f - y * 2.0f;

	const float scale = 250.0f;
	lightPosition.x = x * scale - 278;
	lightPosition.y = y * scale + 278;

	//std::cout << lightPosition.x << " " << lightPosition.y << " " << lightPosition.z << std::endl;
}
