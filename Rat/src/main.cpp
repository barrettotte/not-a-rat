// Display 3D model of rat from Halo 3

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream> // iostream after cmath for M_PI define

#include <glad/glad.h>

#ifndef _glfw3_h_
#include <GLFW/glfw3.h>
#endif

#include "OBJLoader.h"

const char* VERTEX_SHADER_SRC = "#version 330 core\n"
	"layout(location = 0) in vec3 aPos;\n"                           // vertex position
	"layout(location = 1) in vec2 aTexCoord;\n"                      // texture coord
	"layout(location = 2) in vec3 aNormal;\n"                        // vertex normal
	"\n"
	"uniform mat4 model;\n"                                          // model matrix for transform
	"uniform mat4 view;\n"                                           // view matrix for camera
	"uniform mat4 projection;\n"                                     // projection matrix for perspective
	"\n"
	"out vec2 texCoord;\n"                                           // pass texture coord to fragment shader
	"\n"
	"void main() {\n"
	"  gl_Position = projection * view * model * vec4(aPos, 1.0);\n" // apply transformations
	"  texCoord = aTexCoord;\n"                                      // pass texture coords
	"}\0";

const char* FRAGMENT_SHADER_SRC = "#version 330 core\n"
	"out vec4 color;\n"
	"in vec2 texCoord;\n"
	"\n"
	"uniform sampler2D texture1;\n"
	"\n"
	"void main() {\n"
	"  color = texture(texture1, texCoord);\n"
	"}\0";

// create rotation matrix about y-axis
void createRotationMatrixY(float angle, float* matrix) {
	float cosAngle = cos(angle);
	float sinAngle = sin(angle);

	matrix[0] = cosAngle;
	matrix[1] = 0.0f;
	matrix[2] = sinAngle;
	matrix[3] = 0.0f;

	matrix[4] = 0.0f;
	matrix[5] = 1.0f;
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;

	matrix[8] = -sinAngle;
	matrix[9] = 0.0f;
	matrix[10] = cosAngle;
	matrix[11] = 0.0f;

	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	matrix[15] = 1.0f;
}

// create view matrix for camera
void createViewMatrix(float* viewMatrix, float cameraX, float cameraY, float cameraZ) {
	// set camera position
	float centerX = 0.0f;
	float centerY = 0.0f;
	float centerZ = 0.0f;

	// up vector
	float upX = 0.0f;
	float upY = 1.0f;
	float upZ = 0.0f;

	// forward vector
	float fwd[3] = {
		centerX - cameraX, 
		centerY - cameraY, 
		centerZ - cameraZ
	};

	// normalize forward vector
	float fwdLength = sqrt(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);
	fwd[0] /= fwdLength;
	fwd[1] /= fwdLength;
	fwd[2] /= fwdLength;

	// right vector
	float right[3] = {
		upY * fwd[2] - upZ * fwd[1],
		upZ * fwd[0] - upX * fwd[2],
		upX * fwd[1] - upY * fwd[0]
	};

	// normalize right vector
	float rightLength = sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
	right[0] /= rightLength;
	right[1] /= rightLength;
	right[2] /= rightLength;

	// recalculate up vector
	float up[3] = {
		right[1] * fwd[2] - right[2] * fwd[1],
		right[2] * fwd[0] - right[0] * fwd[2],
		right[0] * fwd[1] - right[1] * fwd[0]
	};

	// create the view matrix
	viewMatrix[0] = right[0];
	viewMatrix[1] = up[0];
	viewMatrix[2] = -fwd[0];
	viewMatrix[3] = 0.0f;

	viewMatrix[4] = right[1];
	viewMatrix[5] = up[1];
	viewMatrix[6] = -fwd[1];
	viewMatrix[7] = 0.0f;

	viewMatrix[8] = right[2];
	viewMatrix[9] = up[2];
	viewMatrix[10] = -fwd[2];
	viewMatrix[11] = 0.0f;

	viewMatrix[12] = -(right[0] * cameraX + right[1] * cameraY + right[2] * cameraZ);
	viewMatrix[13] = -(up[0] * cameraX + up[1] * cameraY + up[2] * cameraZ);
	viewMatrix[14] = fwd[0] * cameraX + fwd[1] * cameraY + fwd[2] * cameraZ;
	viewMatrix[15] = 1.0f;
}

// projection matrix for converting 3D coords to 2D coords
void createPerspectiveMatrix(float* projectionMatrix, float fov, float aspect, float near, float far) {
	float tanHalfFOV = tan(fov / 2.0f);

	projectionMatrix[0] = 1.0f / (aspect * tanHalfFOV);
	projectionMatrix[1] = 0.0f;
	projectionMatrix[2] = 0.0f;
	projectionMatrix[3] = 0.0f;

	projectionMatrix[4] = 0.0f;
	projectionMatrix[5] = 1.0f / tanHalfFOV;
	projectionMatrix[6] = 0.0f;
	projectionMatrix[7] = 0.0f;

	projectionMatrix[8] = 0.0f;
	projectionMatrix[9] = 0.0f;
	projectionMatrix[10] = -(far + near) / (far - near);
	projectionMatrix[11] = -1.0f;

	projectionMatrix[12] = 0.0f;
	projectionMatrix[13] = 0.0f;
	projectionMatrix[14] = -(2.0f * far * near) / (far - near);
	projectionMatrix[15] = 0.0f;
}

// create matrix to flip model rightside up
void createFlipMatrixY(float* matrix) {
	memset(matrix, 0, 16 * sizeof(float));
	matrix[0] = 1.0f;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;

	matrix[4] = 0.0f;
	matrix[5] = -1.0f; // flip on y-axis
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;

	matrix[8] = 0.0f;
	matrix[9] = 0.0f;
	matrix[10] = 1.0f;
	matrix[11] = 0.0f;

	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	matrix[15] = 1.0f;
}

// multiply two 4x4 matrices
void multiplyMat4(const float* a, const float* b, float* product) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			product[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] +
				                 a[i * 4 + 1] * b[1 * 4 + j] +
				                 a[i * 4 + 2] * b[2 * 4 + j] +
				                 a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
}

// trigger on every window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

// handle user input
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
		// TODO: display message and don't actually exit
	}
}

// compile and link shaders
GLuint createShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
	
	// compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
	glCompileShader(vertexShader);

	// check if vertex shader compiled successfully
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "Vertex shader compilation failed.\n" << infoLog << std::endl;
		std::exit(-1);
	}

	// compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
	glCompileShader(fragmentShader);

	// check if fragment shader compiled successfully
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "Fragment shader compilation failed.\n" << infoLog << std::endl;
		std::exit(-1);
	}

	// link shaders and create shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// check if linked successfully
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		std::cerr << "Shader linking failed.\n" << infoLog << std::endl;
		std::exit(-1);
	}

	// clean up shaders (linked in program and no longer needed)
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// entry point
int main() {
	std::cout << "Rat initializing..." << std::endl;
	int windowWidth = 640;
	int windowHeight = 480;

	if (!glfwInit()) {
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create window in windowed mode
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Behold. Rat.", NULL, NULL);

	if (!window) {
		std::cout << "ERROR: Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	
	// load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "ERROR: Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// enable 3D depth to render correctly
	glEnable(GL_DEPTH_TEST);

	// enable wireframe
	 //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// create shader program
	GLuint shaderProgram = createShaderProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);

	// load OBJ model
	OBJLoader objLoader;
	if (!objLoader.loadOBJ("../assets/rat.obj")) {
		std::cout << "Failed to load OBJ file" << std::endl;
		return -1;
	}
	objLoader.setupBuffers();

	// model matrix
	float modelMatrix[16];

	// camera settings
	float fov = 45.0f * (M_PI / 180.0f);
	float aspect = 1.0f * windowWidth / windowHeight;
	float near = 0.1f;
	float far = 100.0f;

	float perspectiveMatrix[16];
	createPerspectiveMatrix(perspectiveMatrix, fov, aspect, near, far);

	float rotationMatrix[16];
	createRotationMatrixY(0.0f, rotationMatrix);

	float viewMatrix[16];
	createViewMatrix(viewMatrix, 0.0f, 0.0f, 3.0f);

	float flipMatrix[16];
	createFlipMatrixY(flipMatrix);

	float rotationAngleY = 0.0f;

	// main loop
	while (!glfwWindowShouldClose(window)) {
		// TODO: disable window close

		// input
		processInput(window);

		// clear screen
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set shader program
		glUseProgram(shaderProgram);

		// set texture for shader
		glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

		// update projection and view matrices
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, perspectiveMatrix);

		// update model rotation matrix
		createRotationMatrixY(rotationAngleY, rotationMatrix);
		multiplyMat4(flipMatrix, rotationMatrix, modelMatrix); // flip, then rotate
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix);
		rotationAngleY += 0.03f;

		// render
		objLoader.renderModel();

		// check I/O events and swap rendering buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up
	glfwTerminate();
	return 0;
}
