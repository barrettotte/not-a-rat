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

	// set resource mapping for embedded files
	void setResourceMapping(std::map<std::string, int> resourceMapping);

	// load embedded OBJ from binary
	bool loadEmbeddedObj(const std::string& embeddedFileName);

	// load OBJ file at path
	bool loadObj(const std::string& filePath);

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

	// resource mappings
	std::map<std::string, int> resourceMapping;

	// buffers per context
	std::vector<GLuint> VAOs;

	// materials/textures
	std::map<std::string, Material> materials;
	GLuint textureID;

	// load embedded resource using rc file and resource header definitions
	std::stringstream loadEmbeddedResourceStream(const std::string& fileName);

	// parse OBJ from stream
	bool parseObj(std::stringstream& objStream, const std::string& filePrefix, bool isEmbedded);

	// parse MTL from stream
	bool parseMtl(std::stringstream& mtlStream);

	// create texture from image data
	GLuint createTexture(const std::string& texturePath, unsigned char* imageData, int width, int height, int channels);

	// load MTL file at path
	bool loadMtl(const std::string& filePath);

	// load embedded MTL from binary
	bool loadEmbeddedMtl(const std::string& embeddedFileName);

	// load texture file at path
	GLuint loadTexture(const std::string& filePath);

	// load embedded texture from binary
	GLuint loadEmbeddedTexture(const std::string& embeddedFileName);
};

#endif
