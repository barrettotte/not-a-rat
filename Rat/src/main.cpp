// Display rotating 3D model of rat from Halo 3

#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream> // iostream include after cmath for M_PI definition
#include <map>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OBJLoader.h"
#include "debug.h"
#include "resource.h"

// window settings
const int WINDOW_COUNT = 25;
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

// create mapping between embedded resource ID and filename
std::map<std::string, int> RESOURCE_MAPPING = {
	{"rat.obj", RAT_OBJ},
	{"rat.png", RAT_PNG},
	{"rat.mtl", RAT_MTL}
};

// angle to increment model by each frame
float MODEL_ROTATION_SPEED = 0.03f;

// camera settings
float CAMERA_POS[3] = {-1.0f, -1.5f, 1.5f}; // (X,Y,Z)
float CAMERA_ASPECT = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
float CAMERA_FOV = 20.0f * ((float) M_PI / 180.0f);
float CAMERA_NEAR = 0.1f;
float CAMERA_FAR = 100.0f;

// GLSL for processing vertices
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

// GLSL for handling color
const char* FRAGMENT_SHADER_SRC = "#version 330 core\n"
	"out vec4 color;\n"
	"in vec2 texCoord;\n"
	"\n"
	"uniform sampler2D texture1;\n"
	"\n"
	"void main() {\n"
	"  color = texture(texture1, texCoord);\n"
	"}\0";

// rotation matrix about y-axis
void setRotationMatrixY(float angle, float* matrix) {
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

// view matrix for camera
void setViewMatrix(float* viewMatrix, float cameraX, float cameraY, float cameraZ) {
	float center[3] = {0.0f};
	float up[3] = {0.0f, 1.0f, 0.0f};

	// forward vector
	float fwd[3] = {center[0] - cameraX, center[1] - cameraY, center[2] - cameraZ};

	// normalize forward vector
	float fwdLength = sqrt((fwd[0] * fwd[0]) + (fwd[1] * fwd[1]) + (fwd[2] * fwd[2]));
	fwd[0] /= fwdLength;
	fwd[1] /= fwdLength;
	fwd[2] /= fwdLength;

	// right vector
	float right[3] = {
		(up[1] * fwd[2]) - (up[2] * fwd[1]),
		(up[2] * fwd[0]) - (up[0] * fwd[2]),
		(up[0] * fwd[1]) - (up[1] * fwd[0])
	};

	// normalize right vector
	float rightLength = sqrt((right[0] * right[0]) + (right[1] * right[1]) + (right[2] * right[2]));
	right[0] /= rightLength;
	right[1] /= rightLength;
	right[2] /= rightLength;

	// recalculate up vector
	up[0] = (right[1] * fwd[2]) - (right[2] * fwd[1]);
	up[1] = (right[2] * fwd[0]) - (right[0] * fwd[2]);
	up[2] = (right[0] * fwd[1]) - (right[1] * fwd[0]);

	// build view matrix
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

	viewMatrix[12] = -((right[0] * cameraX) + (right[1] * cameraY) + (right[2] * cameraZ));
	viewMatrix[13] = -((up[0] * cameraX) + (up[1] * cameraY) + (up[2] * cameraZ));
	viewMatrix[14] = (fwd[0] * cameraX) + (fwd[1] * cameraY) + (fwd[2] * cameraZ);
	viewMatrix[15] = 1.0f;
}

// projection matrix for converting 3D coords to 2D coords
void setPerspectiveMatrix(float* projectionMatrix, float fov, float aspect, float camNear, float camFar) {
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
	projectionMatrix[10] = -(camFar + camNear) / (camFar - camNear);
	projectionMatrix[11] = -1.0f;

	projectionMatrix[12] = 0.0f;
	projectionMatrix[13] = 0.0f;
	projectionMatrix[14] = -(2.0f * camFar * camNear) / (camFar - camNear);
	projectionMatrix[15] = 0.0f;
}

// matrix to flip on y-axis
void setFlipMatrixY(float* matrix, float flip) {
	matrix[0] = 1.0f;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;

	matrix[4] = 0.0f;
	matrix[5] = flip; // flip on y-axis
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
			product[i * 4 + j] = (a[i * 4 + 0] * b[0 * 4 + j]) +
				                 (a[i * 4 + 1] * b[1 * 4 + j]) +
				                 (a[i * 4 + 2] * b[2 * 4 + j]) +
				                 (a[i * 4 + 3] * b[3 * 4 + j]);
		}
	}
}

// get random int in range
int getRandomInRange(int min, int max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(min, max);
	return dis(gen);
}

// change background color over time
void setBackgroundColor(float t) {
	float r = (sin(t * 1.0f) * 0.5f) + 0.5f;
	float g = (sin(t * 1.3f) * 0.5f) + 0.5f;
	float b = (sin(t * 1.7f) * 0.5f) + 0.5f;
	glClearColor(r, g, b, 1.0f);
}

// trigger on every window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

// compile and link shaders
GLuint createShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
	GLint status = GL_FALSE;
	const int LOG_SIZE = 512;
	char log[LOG_SIZE];

	DEBUG_STDOUT("Compiling shaders" << std::endl);
	
	// compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
	glCompileShader(vertexShader);

	// check if vertex shader compiled successfully
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glGetShaderInfoLog(vertexShader, LOG_SIZE, 0, log);
		DEBUG_STDERR("ERROR: Vertex shader compilation failed.\n" << log << std::endl);
		std::exit(-1);
	}
	DEBUG_STDOUT("  Compiled vertex shader" << std::endl);

	// compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(fragmentShader);

	// check if fragment shader compiled successfully
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glGetShaderInfoLog(fragmentShader, LOG_SIZE, 0, log);
		DEBUG_STDERR("ERROR: Fragment shader compilation failed.\n" << log << std::endl);
		std::exit(-1);
	}
	DEBUG_STDOUT("  Compiled fragment shader" << std::endl);

	// link shaders and create shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// check if linked successfully
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		DEBUG_STDERR("ERROR: Shader linking failed.\n" << log << std::endl);
		std::exit(-1);
	}
	DEBUG_STDOUT("  Linked shaders" << std::endl);

	// clean up shaders (linked in program and no longer needed)
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// entry point
int main(int argc, char** argv) {
	if (!glfwInit()) {
		DEBUG_STDERR("ERROR: Failed to init GLFW" << std::endl);
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// get primary monitor resolution
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
	const int screenWidth = videoMode->width;
	const int screenHeight = videoMode->height;
	DEBUG_STDOUT("Primary monitor is " << screenWidth << "x" << screenHeight << std::endl;)

	// create windows
	std::vector<GLFWwindow*> windows;
	for (int i = 0; i < WINDOW_COUNT; i++) {
		std::string title = "[" + std::to_string(i) + "] YOU JUST GOT RATTED!";
		GLFWwindow* shared = (i == 0) ? NULL : windows[0];
		GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str(), NULL, shared);
		
		if (!window) {
			DEBUG_STDERR("ERROR: Failed to create GLFW window " << i << std::endl);
			glfwTerminate();
			return -1;
		}
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		// set window position randomly
		int randX = getRandomInRange(10, screenWidth - WINDOW_WIDTH - 10);
		int randY = getRandomInRange(10, screenHeight - WINDOW_HEIGHT - 10);
		glfwSetWindowPos(window, randX, randY);

		windows.push_back(window);
	}

	DEBUG_STDOUT("Created " << windows.size() << " window(s)" << std::endl);

	for (int i = 1; i < windows.size(); i++) {
		if (glfwGetWindowAttrib(windows.at(i), GLFW_CONTEXT_VERSION_MAJOR) == glfwGetWindowAttrib(windows.at(0), GLFW_CONTEXT_VERSION_MAJOR)) {
			DEBUG_STDOUT("Context sharing successful for window " << i << std::endl);
		}
		else {
			DEBUG_STDERR("Context sharing failed for window " << i << std::endl);
			std::exit(-1);
		}
	}

	// start with first window
	glfwMakeContextCurrent(windows[0]);

	// load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		DEBUG_STDERR("ERROR: Failed to initialize GLAD" << std::endl);
		return -1;
	}

	// enable wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// create shader program
	GLuint shaderProgram = createShaderProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
	CHECK_ERROR_GL();

	OBJLoader objLoader;
	objLoader.setResourceMapping(RESOURCE_MAPPING);

	// load OBJ model from filesystem
	//if (!objLoader.loadObj("assets/rat.obj")) {
	//	DEBUG_STDERR("ERROR: Failed to load OBJ file" << std::endl);
	//	return -1;
	//}

	// load embedded OBJ model
	if (!objLoader.loadEmbeddedObj("rat.obj")) {
		DEBUG_STDERR("ERROR: Failed to load OBJ file" << std::endl);
		return -1;
	}

	// configure each window context
	for (int i = 0; i < windows.size(); i++) {

		// setup buffers for each context
		glfwMakeContextCurrent(windows.at(i));
		objLoader.setupBuffers(i);

		// misc config
		glEnable(GL_DEPTH_TEST); // enable 3D depth to render correctly
		glfwSwapInterval(1);     // enable VSync
	}

	// init matrices
	float perspectiveMatrix[16];
	setPerspectiveMatrix(perspectiveMatrix, CAMERA_FOV, CAMERA_ASPECT, CAMERA_NEAR, CAMERA_FAR);

	float viewMatrix[16];
	setViewMatrix(viewMatrix, CAMERA_POS[0], CAMERA_POS[1], CAMERA_POS[2]);

	float rotationMatrix[16];
	setRotationMatrixY(0.0f, rotationMatrix);

	// OBJ gets exported upside down, fix it by flipping on Y-axis
	float flipMatrix[16];
	setFlipMatrixY(flipMatrix, -1.0f);

	float modelMatrix[16];
	float rotationAngleY = 0.0f;
	float t = 0.0f;
	GLFWwindow* window = windows[0];

	// main loop (does not allow normal exit)
	while (true) {
		t = (float) glfwGetTime();

		for (int i = 0; i < windows.size(); i++) {
			window = windows[i];
			glfwMakeContextCurrent(window);
			int windowOffset = i; // make each window slightly different

			// clear screen
			setBackgroundColor(t + windowOffset);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// set shader program
			glUseProgram(shaderProgram);
			glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

			// set model rotation matrix
			setRotationMatrixY(rotationAngleY + windowOffset, rotationMatrix);
			multiplyMat4(flipMatrix, rotationMatrix, modelMatrix); // flip, then rotate

			// update matrices for shader
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, perspectiveMatrix);

			// render
			objLoader.renderModel(i);

			// swap rendering buffers
			glfwSwapBuffers(window);

			// check I/O events
			glfwPollEvents();
		}

		// update model rotation
		rotationAngleY += MODEL_ROTATION_SPEED;
	}

	// clean up
	for (int i = 0; i < windows.size(); i++) {
		glfwDestroyWindow(windows[i]);
	}
	glfwTerminate();

	return 0;
}
