// parse and load OBJ file

#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "OBJLoader.h"
#include "debug.h"
#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

OBJLoader::OBJLoader() : VAOs(0), VBO(0), EBO(0), textureID(0), materials{} {
	// nop
}

OBJLoader::~OBJLoader() {
	for (int i = 0; i < VAOs.size(); i++) {
		glDeleteVertexArrays(1, &VAOs[i]);
	}
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

// set resource mapping for embedded files
void OBJLoader::setResourceMapping(std::map<std::string, int> resMapping) {
	resourceMapping = resMapping;
}

// load embedded resource using rc file and resource header definitions
std::stringstream OBJLoader::loadEmbeddedResourceStream(const std::string& fileName) {
	
	// check if mapping existings for embedded file
	if (!resourceMapping.count(fileName)) {
		DEBUG_STDERR("Could not find resource mapping for \"" << fileName << "\"" << std::endl);
		std::exit(-1);
	}

	// get resource ID from embedded file name
	int resourceId = resourceMapping[fileName];
	const WCHAR* resourceName = MAKEINTRESOURCE(resourceId);
	HRSRC hResource = FindResource(NULL, resourceName, RT_RCDATA);

	if (hResource == NULL) {
		DEBUG_STDERR("ERROR: Failed to find resource \"" << resourceName << "\"" << std::endl);
		std::exit(-1);
	}

	HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
	if (hLoadedResource == NULL) {
		DEBUG_STDERR("ERROR: Failed to load resource \"" << resourceName << "\"" << std::endl);
		std::exit(-1);
	}

	// lock resource to access data
	void* pResourceData = LockResource(hLoadedResource);
	DWORD resourceSize = SizeofResource(NULL, hResource);

	if (pResourceData == NULL || resourceSize == 0) {
		DEBUG_STDERR("ERROR: Resource data is invalid " << resourceName << std::endl);
		std::exit(-1);
	}
	std::stringstream resourceStream(std::string(static_cast<char*>(pResourceData), resourceSize));

	return resourceStream;
}

// parse and load OBJ model
bool OBJLoader::parseObj(std::stringstream& objStream, const std::string& filePrefix, bool isEmbedded) {
	std::string line;
	int lineNum = 1;
	std::string currentMaterialName;

	// parse OBJ commands
	while (std::getline(objStream, line)) {
		std::istringstream iss(line);
		std::string cmd;

		iss >> cmd;

		if (cmd == "mtllib") {
			std::string mtlFileName;
			iss >> mtlFileName;
			std::string mtlPath = filePrefix + mtlFileName;

			bool success = (isEmbedded) ? loadEmbeddedMtl(mtlPath) : loadMtl(mtlPath);
			if (!success) {
				DEBUG_STDERR("  ERROR: MTL file " << mtlPath << " failed to parse" << std::endl);
			}
		}
		else if (cmd == "usemtl") {
			iss >> currentMaterialName;

			// load texture for material
			if (materials.count(currentMaterialName) > 0) {
				std::string texturePath = filePrefix + materials[currentMaterialName].texturePath;
				textureID = (isEmbedded) ? loadEmbeddedTexture(texturePath) : loadTexture(texturePath);
			}
		}
		else if (cmd == "v") {
			VertexPos vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (cmd == "vt") {
			TexCoord texCoord;
			iss >> texCoord.u >> texCoord.v; // UV coordinates
			texCoord.v = 1.0f - texCoord.v; // flip, OBJ's UV map seemed to export upside down
			texCoords.push_back(texCoord);
		}
		else if (cmd == "vn") {
			Normal normal;
			iss >> normal.nx >> normal.ny >> normal.nz; // normals
			normals.push_back(normal);
		}
		else if (cmd == "f") {
			Face face;
			char separator;

			for (int i = 0; i < 3; i++) {
				iss >> face.vIdx[i] >> separator >> face.tIdx[i] >> separator >> face.nIdx[i];
				// convert from one to zero indexing
				face.vIdx[i]--;
				face.tIdx[i]--;
				face.nIdx[i]--;
			}
			faces.push_back(face);
		}
		else if (cmd == "#" || cmd == " " || cmd == "") {
			// comment or empty line, do nothing
		}
		else {
			DEBUG_STDOUT("  Unsupported command \"" << cmd << "\" on line " << lineNum << std::endl);
		}
		lineNum++;
	}
	return true;
}

// parse MTL from stream
bool OBJLoader::parseMtl(std::stringstream& mtlStream) {
	std::string line;
	int lineNum = 1;
	Material currentMaterial;

	// parse MTL commands
	while (std::getline(mtlStream, line)) {
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		if (cmd == "newmtl") {
			if (!currentMaterial.name.empty()) {
				materials[currentMaterial.name] = currentMaterial;
			}
			iss >> currentMaterial.name;
		}
		else if (cmd == "map_Kd") {
			iss >> currentMaterial.texturePath; // diffuse map
		}
		else if (cmd == "#" || cmd == " " || cmd == "") {
			// comment or empty line, do nothing
		}
		else {
			DEBUG_STDOUT("    Unsupported command \"" << cmd << "\" on line " << lineNum << std::endl);
		}
		lineNum++;
	}

	// set current material
	if (!currentMaterial.name.empty()) {
		materials[currentMaterial.name] = currentMaterial;
	}
	return true;
}

// set texture from stream
GLuint OBJLoader::createTexture(const std::string& texturePath, unsigned char* imageData, int width, int height, int channels) {
	GLuint textureID;
	glGenTextures(1, &textureID);

	if (imageData) {
		DEBUG_STDOUT("  Loaded texture " << texturePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set texture wrapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// set texture filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// determine texture format
		GLenum fmt = (channels == 1) ? GL_RED : (channels == 3) ? GL_RGB : GL_RGBA;

		// load texture data
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, imageData);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		DEBUG_STDERR("  ERROR: Failed to load texture " << std::endl);
	}
	return textureID;
}

// load embedded OBJ model with file name
bool OBJLoader::loadEmbeddedObj(const std::string& embeddedFileName) {
	std::stringstream objStream = loadEmbeddedResourceStream(embeddedFileName);
	DEBUG_STDOUT("Loaded embedded OBJ " << embeddedFileName << std::endl);
	return parseObj(objStream, "", true);
}

// load embedded MTL with file name
bool OBJLoader::loadEmbeddedMtl(const std::string& embeddedFileName) {
	std::stringstream mtlStream = loadEmbeddedResourceStream(embeddedFileName);
	DEBUG_STDOUT("Loaded embedded MTL " << embeddedFileName << std::endl);
	return parseMtl(mtlStream);
}

// load embedded texture with file name
GLuint OBJLoader::loadEmbeddedTexture(const std::string& embeddedFileName) {
	int width = 0;
	int height = 0;
	int channels = 0;

	std::stringstream textureStream = loadEmbeddedResourceStream(embeddedFileName);
	std::string textureData = textureStream.str();
	const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(textureData.data());
	size_t dataSize = textureData.size();

	unsigned char* imageData = stbi_load_from_memory(dataPtr, static_cast<int>(dataSize), &width, &height, &channels, 0);
	GLuint textureID = createTexture(embeddedFileName, imageData, width, height, channels);
	stbi_image_free(imageData);

	return textureID;
}

// load OBJ model at file path
bool OBJLoader::loadObj(const std::string& objPath) {
	std::ifstream objFile(objPath);

	if (!objFile.is_open()) {
		DEBUG_STDERR("ERROR: Failed to open OBJ file " << objPath << std::endl);
		return false;
	}
	DEBUG_STDOUT("Loaded object " << objPath << std::endl);

	// get directory from OBJ file path
	std::string::size_type slashIndex = objPath.find_last_of("/\\");
	std::string dir = (slashIndex != std::string::npos) ? objPath.substr(0, slashIndex) : "";

	// process stream
	std::stringstream buffer;
	buffer << objFile.rdbuf();
	bool success = parseObj(buffer, dir + "/", false);
	objFile.close();

	return success;	
}

// load materials from file path
bool OBJLoader::loadMtl(const std::string& mtlPath) {
	std::ifstream mtlFile(mtlPath);

	if (!mtlFile.is_open()) {
		DEBUG_STDERR("  ERROR: Failed to open MTL file " << mtlPath << std::endl);
		return false;
	}
	DEBUG_STDOUT("  Loaded material " << mtlPath << std::endl);

	// process stream
	std::stringstream buffer;
	buffer << mtlFile.rdbuf();
	bool success = parseMtl(buffer);
	mtlFile.close();

	return success;
}

// load texture in OpenGL
GLuint OBJLoader::loadTexture(const std::string& texturePath) {
	int width = 0;
	int height = 0;
	int channels = 0;

	unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
	GLuint textureID = createTexture(texturePath, data, width, height, channels);
	stbi_image_free(data);

	return textureID;
}

// setup OpenGL buffers
void OBJLoader::setupBuffers(int contextIdx) {
	std::vector<float> vertexData;
	std::vector<GLsizei> indices;

	DEBUG_STDOUT("Setting up buffers for context " << contextIdx << std::endl);

	// setup vertex data
	for (const auto& face : faces) {
		for (int i = 0; i < 3; i++) {
			VertexPos v = vertices[face.vIdx[i]];
			vertexData.push_back(v.x);
			vertexData.push_back(v.y);
			vertexData.push_back(v.z);

			TexCoord t = texCoords[face.tIdx[i]];
			vertexData.push_back(t.u);
			vertexData.push_back(t.v);

			Normal n = normals[face.nIdx[i]];
			vertexData.push_back(n.nx);
			vertexData.push_back(n.ny);
			vertexData.push_back(n.nz);

			indices.push_back((GLsizei) indices.size());
		}
	}

	// verify vertex data exists
	if (vertexData.empty()) {
		DEBUG_STDERR("ERROR: No vertex data to send to GPU" << std::endl);
		std::exit(-1);
	}
	DEBUG_STDOUT("Setup vertex data" << std::endl);

	// generate and bind VAO
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	VAOs.push_back(VAO);

	// generate VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_STATIC_DRAW);

	// position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	
	// texture coordinate attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	
	// normal attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	// generate EBO
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

	// unbind VAO
	glBindVertexArray(0);
}

// render the OBJ
void OBJLoader::renderModel(int contextIdx) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBindVertexArray(VAOs[contextIdx]);
	glDrawElements(GL_TRIANGLES, (GLsizei) faces.size() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0); // unbind VAO
}
