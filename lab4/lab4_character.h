#ifndef LAB4_CHARACTER_H
#define LAB4_CHARACTER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iomanip>
// GLTF model loader
#include <tiny_gltf.h>

#include <render/shader.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


static int windowWidth = 1024;
static int windowHeight = 768;


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Camera
static float FoV = 45.0f;
static float zNear = 100.0f;
static float zFar = 1500.0f;
std::map<std::string, GLuint> textureCache;
std::map<std::string, tinygltf::Model> modelCache;

// Lighting
static glm::vec3 _lightIntensity(5e6f, 5e6f, 5e6f);
static glm::vec3 lightPosition(-275.0f, 500.0f, 800.0f);

// Animation
static bool playAnimation = true;
static float playbackSpeed = 2.0f;

struct MyBot {
	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint jointMatricesID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;

	tinygltf::Model model;

	std::map<int, GLuint> vbos;

	// Each VAO corresponds to each mesh primitive in the GLTF model
	struct PrimitiveObject {
		GLuint vao;
	};
	std::vector<PrimitiveObject> primitiveObjects;
	std::map<std::string, std::vector<PrimitiveObject>> primitiveObjectsByName;
	std::map<int, GLuint> textures;

	// Skinning
	struct SkinObject {
		// Transforms the geometry into the space of the respective joint
		std::vector<glm::mat4> inverseBindMatrices;

		// Transforms the geometry following the movement of the joints
		std::vector<glm::mat4> globalJointTransforms;

		// Combined transforms
		std::vector<glm::mat4> jointMatrices;
	};
	std::vector<SkinObject> skinObjects;
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(40.0,40.0,40.0);
	float angle = 0.0f;

	// Animation
	struct SamplerObject {
		std::vector<float> input;
		std::vector<glm::vec4> output;
		int interpolation;
	};
	struct ChannelObject {
		int sampler;
		std::string targetPath;
		int targetNode;
	};
	struct AnimationObject {
		std::vector<SamplerObject> samplers;	// Animation data
	};
	std::vector<AnimationObject> animationObjects;

	GLuint _LoadTextureTileBox(const char* textureFilePath) {
		int width, height, channels;
		unsigned char* img = stbi_load(textureFilePath, &width, &height, &channels, 4);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
					 GL_RGBA, GL_UNSIGNED_BYTE, img);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(img); // Free the image memory after uploading
		return texture;
	}

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		glm::mat4 transform(1.0f);

		if (node.matrix.size() == 16) {
			transform = glm::make_mat4(node.matrix.data());
		} else {
			if (node.translation.size() == 3) {
				transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
			}
			if (node.rotation.size() == 4) {
				glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				transform *= glm::mat4_cast(q);
			}
			if (node.scale.size() == 3) {
				transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			}
		}
		return transform;
	}

	void computeLocalNodeTransform(const tinygltf::Model& model,
	int nodeIndex,
	std::vector<glm::mat4> &localTransforms)
	{
		const tinygltf::Node& node = model.nodes[nodeIndex];
		localTransforms[nodeIndex] = getNodeTransform(node);
	}

	void computeGlobalNodeTransform(const tinygltf::Model& model,
	const std::vector<glm::mat4>& localTransforms,
	int nodeIndex, const glm::mat4& parentTransform,
	std::vector<glm::mat4>& globalTransforms, std::vector<bool> &reached)
	{
		if (reached[nodeIndex]) return;
		else reached[nodeIndex] = true;
		globalTransforms[nodeIndex] = parentTransform * localTransforms[nodeIndex];
		for (int child : model.nodes[nodeIndex].children) {
			computeGlobalNodeTransform(model, localTransforms, child, globalTransforms[nodeIndex], globalTransforms, reached);
		}
	}

	std::vector<SkinObject> prepareSkinning(const tinygltf::Model &model) {
		std::vector<SkinObject> skinObjects;

		// In our Blender exporter, the default number of joints that may influence a vertex is set to 4, just for convenient implementation in shaders.

		for (size_t i = 0; i < model.skins.size(); i++) {
			SkinObject skinObject;

			const tinygltf::Skin &skin = model.skins[i];

			// Read inverseBindMatrices
			const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
			assert(accessor.type == TINYGLTF_TYPE_MAT4);
			const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			const float *ptr = reinterpret_cast<const float *>(
            	buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);

			skinObject.inverseBindMatrices.resize(accessor.count);
			for (size_t j = 0; j < accessor.count; j++) {
				float m[16];
				memcpy(m, ptr + j * 16, 16 * sizeof(float));
				skinObject.inverseBindMatrices[j] = glm::make_mat4(m);
			}

			assert(skin.joints.size() == accessor.count);

			skinObject.globalJointTransforms.resize(skin.joints.size());
			skinObject.jointMatrices.resize(skin.joints.size());

			// ----------------------------------------------
			// TODO: your code here to compute joint matrices
			// ----------------------------------------------#
			std::vector<glm::mat4> localTransforms;
			localTransforms.resize(skin.joints.size());
			computeLocalNodeTransform(model, model.skins[0].joints[0], localTransforms);
			skinObject.globalJointTransforms.resize(skin.joints.size(), glm::mat4(1.0f));
			std::vector<bool> reached(skinObject.globalJointTransforms.size(), false);
			computeGlobalNodeTransform(model, localTransforms,model.skins[0].joints[0],
				glm::mat4(1.0f), skinObject.globalJointTransforms, reached);
			updateSkinning(skinObject.globalJointTransforms);





			// ----------------------------------------------

			skinObjects.push_back(skinObject);
		}
		return skinObjects;
	}

	int findKeyframeIndex(const std::vector<float>& times, float animationTime)
	{
		int left = 0;
		int right = times.size() - 1;

		while (left <= right) {
			int mid = (left + right) / 2;

			if (mid + 1 < times.size() && times[mid] <= animationTime && animationTime < times[mid + 1]) {
				return mid;
			}
			else if (times[mid] > animationTime) {
				right = mid - 1;
			}
			else { // animationTime >= times[mid + 1]
				left = mid + 1;
			}
		}

		// Target not found
		return times.size() - 2;
	}

	std::vector<AnimationObject> prepareAnimation(const tinygltf::Model &model)
	{
		std::vector<AnimationObject> animationObjects;
		for (const auto &anim : model.animations) {
			AnimationObject animationObject;

			for (const auto &sampler : anim.samplers) {
				SamplerObject samplerObject;

				const tinygltf::Accessor &inputAccessor = model.accessors[sampler.input];
				const tinygltf::BufferView &inputBufferView = model.bufferViews[inputAccessor.bufferView];
				const tinygltf::Buffer &inputBuffer = model.buffers[inputBufferView.buffer];

				assert(inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				assert(inputAccessor.type == TINYGLTF_TYPE_SCALAR);

				// Input (time) values
				samplerObject.input.resize(inputAccessor.count);

				const unsigned char *inputPtr = &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset];
				const float *inputBuf = reinterpret_cast<const float*>(inputPtr);

				// Read input (time) values
				int stride = inputAccessor.ByteStride(inputBufferView);
				for (size_t i = 0; i < inputAccessor.count; ++i) {
					samplerObject.input[i] = *reinterpret_cast<const float*>(inputPtr + i * stride);
				}

				const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
				const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
				const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

				assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
				const float *outputBuf = reinterpret_cast<const float*>(outputPtr);

				int outputStride = outputAccessor.ByteStride(outputBufferView);

				// Output values
				samplerObject.output.resize(outputAccessor.count);

				for (size_t i = 0; i < outputAccessor.count; ++i) {

					if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
						memcpy(&samplerObject.output[i], outputPtr + i * 3 * sizeof(float), 3 * sizeof(float));
					} else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
						memcpy(&samplerObject.output[i], outputPtr + i * 4 * sizeof(float), 4 * sizeof(float));
					} else {
						std::cout << "Unsupport accessor type ..." << std::endl;
					}

				}

				animationObject.samplers.push_back(samplerObject);
			}

			animationObjects.push_back(animationObject);
		}
		return animationObjects;
	}

	void updateAnimation(
		const tinygltf::Model &model,
		const tinygltf::Animation &anim,
		const AnimationObject &animationObject,
		float time,
		std::vector<glm::mat4> &nodeTransforms)
	{
		// There are many channels so we have to accumulate the transforms
		for (const auto &channel : anim.channels) {

			int targetNodeIndex = channel.target_node;
			const auto &sampler = anim.samplers[channel.sampler];

			// Access output (value) data for the channel
			const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
			const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
			const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

			// Calculate current animation time (wrap if necessary)
			const std::vector<float> &times = animationObject.samplers[channel.sampler].input;
			float animationTime = fmod(time, times.back());

			// ----------------------------------------------------------
			// TODO: Find a keyframe for getting animation data
			// ----------------------------------------------------------
			int keyframeIndex = findKeyframeIndex(times, animationTime);
			float t = (animationTime - times[keyframeIndex]) /
					  (times[keyframeIndex + 1] - times[keyframeIndex]);

			const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
			const float *outputBuf = reinterpret_cast<const float*>(outputPtr);

			// -----------------------------------------------------------
			// TODO: Add interpolation for smooth animation
			// -----------------------------------------------------------
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

	void updateSkinning(const std::vector<glm::mat4> &globalTransforms) {
		for (size_t i = 0; i < skinObjects.size(); i++) {
			SkinObject &skin = skinObjects[i];
			for (size_t j = 0; j < skin.globalJointTransforms.size(); j++) {
				skin.jointMatrices[j] = globalTransforms[model.skins[0].joints[j]] * skin.inverseBindMatrices[j];
			}
		}
	}

	void update(float time) {
		// Comment out the return statement for animation to run
		// return;

		if (model.animations.size() > 0) {
			const tinygltf::Animation &animation = model.animations[0];
			const AnimationObject &animationObject = animationObjects[0];
			const tinygltf::Skin &skin = model.skins[0];

			// Create and initialize nodeTransforms for local transforms
			std::vector<glm::mat4> nodeTransforms(skin.joints.size(), glm::mat4(1.0f));

			// Update nodeTransforms based on animation data
			updateAnimation(model, animation, animationObject, time, nodeTransforms);

			// Recompute global transforms for the skeleton starting from the root node
			int rootNodeIndex = skin.joints[0]; // Root joint index
			std::vector<bool> reached(skinObjects[0].globalJointTransforms.size(), false);
			computeGlobalNodeTransform(model, nodeTransforms, skin.joints[0], glm::mat4(), skinObjects[0].globalJointTransforms, reached);
			for (int nodeIndex = 0; nodeIndex < skinObjects[0].globalJointTransforms.size(); nodeIndex++)
				computeGlobalNodeTransform(model, nodeTransforms, nodeIndex, glm::mat4(), skinObjects[0].globalJointTransforms, reached);
			updateSkinning(skinObjects[0].globalJointTransforms);

		}

	}

	bool loadModel(tinygltf::Model &model, const char *filename) {
		if (modelCache.find(std::string(filename)) != modelCache.end()) {
			this->model = modelCache[std::string(filename)];
			return true;
		}
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

		if (!res)
			std::cout << "Failed to load glTF: " << filename << std::endl;
		else
			std::cout << "Loaded glTF: " << filename << std::endl;

		modelCache[std::string(filename)] = model;

		return res;
	}

	void initialize(const char* filePath) {
		// Modify your path if needed
		if (!loadModel(model, filePath)) {
			return;
		}

		// Prepare buffers for rendering
		primitiveObjects = bindModel(model);

		// Prepare joint matrices
		skinObjects = prepareSkinning(model);

		// Prepare animation data
		animationObjects = prepareAnimation(model);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromFile("../lab4/shader/bot.vert", "../lab4/shader/bot.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for GLSL variables
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		jointMatricesID = glGetUniformLocation(programID, "jointMatrices");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
	}

	void bindMesh(std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {




		// Each mesh can contain several primitives (or parts), each we need to
		// bind to an OpenGL vertex array object
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			for (auto &attrib : primitive.attributes) {
				tinygltf::Accessor accessor = model.accessors[attrib.second];
				int byteStride =
					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attrib.first.compare("POSITION") == 0) vaa = 0;
				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (attrib.first.compare("JOINTS_0") == 0) vaa = 3;
				if (attrib.first.compare("WEIGHTS_0") == 0) vaa = 4;
				if (vaa > -1) {
					glEnableVertexAttribArray(vaa);
					glVertexAttribPointer(vaa, size, accessor.componentType,
										accessor.normalized ? GL_TRUE : GL_FALSE,
										byteStride, BUFFER_OFFSET(accessor.byteOffset));
				} else {
					std::cout << "vaa missing: " << attrib.first << std::endl;
				}
			}

			// Record VAO for later use
			PrimitiveObject primitiveObject;
			primitiveObject.vao = vao;

			primitiveObjectsByName[mesh.name].push_back(primitiveObject);

			glBindVertexArray(0);
		}
	}

	void bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects,
						tinygltf::Model &model,
						tinygltf::Node &node) {
		// Bind buffers for the current mesh at the node
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			bindMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}

		// Recursive into children nodes
		for (size_t i = 0; i < node.children.size(); i++) {
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}

	std::vector<PrimitiveObject> bindModel(tinygltf::Model &model) {
		for (size_t i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView &bufferView = model.bufferViews[i];

			int target = bufferView.target;

			if (bufferView.target == 0) {
				// The bufferView with target == 0 in our model refers to
				// the skinning weights, for 25 joints, each 4x4 matrix (16 floats), totaling to 400 floats or 1600 bytes.
				// So it is considered safe to skip the warning.
				//std::cout << "WARN: bufferView.target is zero" << std::endl;
				continue;
			}

			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(target, vbo);
			glBufferData(target, bufferView.byteLength,
						&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

			vbos[i] = vbo;
		}
		std::vector<PrimitiveObject> primitiveObjects;
		for (size_t i = 0; i < model.images.size(); ++i) {
			std::string uri = "../lab4/model/buildings/" + model.images[i].uri;
			if (textureCache.find(std::string(uri)) != textureCache.end()) {
				this->textures[i] = textureCache[std::string(uri)];
			}
			else {
				this->textures[i] = _LoadTextureTileBox(uri.c_str());
				textureCache[std::string(uri)] = textures[i];
			}

		}

		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}


		return primitiveObjects;
	}

	void drawMesh(const std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {

		for (size_t i = 0; i < mesh.primitives.size(); ++i)
		{
			auto & primitiveObject = primitiveObjectsByName.at(mesh.name).at(i);
			GLuint vao = primitiveObjectsByName.at(mesh.name).at(i).vao;

			glBindVertexArray(vao);

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
			if (primitive.material >= 0) {
				glUniform1i(glGetUniformLocation(programID, "useTexture"), 0);
				tinygltf::Material material = model.materials[primitive.material];
				if(material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->textures[material.pbrMetallicRoughness.baseColorTexture.index]);
					glUniform1i(glGetUniformLocation(programID, "useTexture"), 1);
				}
				glm::vec3 materialColor = {material.pbrMetallicRoughness.baseColorFactor[0],
					material.pbrMetallicRoughness.baseColorFactor[1],
					material.pbrMetallicRoughness.baseColorFactor[2]};
				glUniform3fv(glGetUniformLocation(programID, "material"), 1, &materialColor[0]);
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, indexAccessor.count,
						indexAccessor.componentType,
						BUFFER_OFFSET(indexAccessor.byteOffset));

			glBindVertexArray(0);
		}
	}

	void drawModelNodes(const std::vector<PrimitiveObject>& primitiveObjects,
						tinygltf::Model &model, tinygltf::Node &node) {
		// Draw the mesh at the node, and recursively do so for children nodes
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			auto transform = getNodeTransform(node);
			glUniformMatrix4fv(glGetUniformLocation(programID, "nodeMatrix"), 1, GL_FALSE, &transform[0][0]);
			drawMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); i++) {
			drawModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}
	void drawModel(const std::vector<PrimitiveObject>& primitiveObjects,
				tinygltf::Model &model) {
		// Draw all nodes
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		// Set camera
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0,1,0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		GLuint buffer_handle;
		GLuint second_buffer_handle;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(programID, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
		if(skinObjects.size() != 0) {
			if(skinObjects[0].jointMatrices.size() > 1000) {

				glGenBuffers(1, &buffer_handle);
				glBindBuffer(GL_UNIFORM_BUFFER, buffer_handle);
				glBufferData(GL_UNIFORM_BUFFER, 1000 * sizeof(glm::mat4), skinObjects[0].jointMatrices.data(), GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(programID, "FirstHalf"), buffer_handle);

				//get the index location and size of the block
				glGenBuffers(1, &second_buffer_handle);
				glBindBuffer(GL_UNIFORM_BUFFER, second_buffer_handle);
				glBufferData(GL_UNIFORM_BUFFER, (skinObjects[0].jointMatrices.size() - 1000) * sizeof(glm::mat4), skinObjects[0].jointMatrices.data() + 1000, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(programID, "SecondHalf"), second_buffer_handle);
			} else {

			}
		} else {
			std::vector <glm::mat4> jointMatrices(1000, glm::mat4(1.0f));
			glGenBuffers(1, &buffer_handle);
			glBindBuffer(GL_UNIFORM_BUFFER, buffer_handle);
			glBufferData(GL_UNIFORM_BUFFER, 1000 * sizeof(glm::mat4), jointMatrices.data(), GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(programID, "FirstHalf"), buffer_handle);
		}


		// -----------------------------------------------------------------
		// TODO: Set animation data for linear blend skinning in shader
		// -----------------------------------------------------------------
		//glUniformMatrix4fv(jointMatricesID, 25, GL_FALSE, &skinObjects[0].jointMatrices[0][0][0]);




		// -----------------------------------------------------------------

		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &_lightIntensity[0]);

		// Draw the GLTF model
		drawModel(primitiveObjects, model);
		glDeleteBuffers(1, &buffer_handle);
		glDeleteBuffers(1, &second_buffer_handle);
	}

	void cleanup() {
		glDeleteProgram(programID);
	}
};


#endif
