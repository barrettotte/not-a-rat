#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <string>
#include <map>
#include <vector>

#include <glad/glad.h>

struct VertexPos {
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
	void setupBuffers(int contextIdx);

	// render model
	void renderModel(int contextIdx);

private:
	// OBJ model data
	std::vector<VertexPos> vertices;
	std::vector<TexCoord> texCoords;
	std::vector<Normal> normals;
	std::vector<Face> faces;

	// shared buffers
	GLuint VBO, EBO;

	// buffers per context
	std::vector<GLuint> VAOs;

	// materials/textures
	std::map<std::string, Material> materials;
	GLuint textureID;

	// load materials from .mtl file
	bool loadMaterials(const std::string& filePath);

	// load texture in OpenGL
	GLuint loadTexture(const std::string& texturePath);
};

#endif
