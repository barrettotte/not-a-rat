#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <string>
#include <map>
#include <vector>

#include <glad/glad.h>

struct Vertex {
	float x, y, z;
};

struct TexCoord {
	float u, v;
};

struct Normal {
	float nx, ny, nz;
};

struct Face {
	unsigned int vIdx[3], tIdx[3], nIdx[3];
};

struct Material {
	std::string name;
	std::string texturePath;
};

class OBJLoader {

public:
	OBJLoader();
	~OBJLoader();

	// load OBJ file at path
	bool loadOBJ(const std::string& filePath);

	// setup OpenGL buffers
	void setupBuffers();

	// render model
	void renderModel();

	// load materials from .mtl file
	bool loadMaterials(const std::string& filePath);

	// load texture in OpenGL
	GLuint loadTexture(const std::string& texturePath);

private:
	// OBJ model data
	std::vector<Vertex> vertices;
	std::vector<TexCoord> texCoords;
	std::vector<Normal> normals;
	std::vector<Face> faces;

	// OpenGL buffers
	GLuint VAO, VBO, EBO;

	// materials/textures
	std::map<std::string, Material> materials;
	GLuint textureID;
};

#endif
