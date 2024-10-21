// load OBJ file

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "OBJLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

OBJLoader::OBJLoader() : VAO(0), VBO(0), EBO(0), textureID(0) {
	// nop
}

OBJLoader::~OBJLoader() {
	// cleanup buffers
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

// parse and load OBJ model at file path
bool OBJLoader::loadOBJ(const std::string& filePath) {
	std::ifstream objFile(filePath);

	if (!objFile.is_open()) {
		std::cerr << "Failed to open OBJ file: " << filePath << std::endl;
		return false;
	}

	// get directory from OBJ file path
	std::string::size_type slashIndex = filePath.find_last_of("/\\");
	std::string objDirStr = (slashIndex != std::string::npos) ? filePath.substr(0, slashIndex) : "";

	std::string line;
	std::string currentMaterialName;

	while (std::getline(objFile, line)) {
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		if (cmd == "mtllib") {
			// load material reference
			std::string mtlFileName;
			iss >> mtlFileName;
			std::string mtlFilePath = objDirStr + "/" + mtlFileName;
			loadMaterials(mtlFilePath);
		}
		else if (cmd == "usemtl") {
			// load texture for material
			iss >> currentMaterialName;

			// load texture for material
			if (materials.count(currentMaterialName) > 0) {
				textureID = loadTexture(objDirStr + "/" + materials[currentMaterialName].texturePath);
			}
		}
		else if (cmd == "v") {
			// vertex position

			Vertex vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (cmd == "vt") {
			// texture coordinate

			TexCoord texCoord;
			iss >> texCoord.u >> texCoord.v;
			texCoord.v = 1.0f - texCoord.v; // flip
			texCoords.push_back(texCoord);
		}
		else if (cmd == "vn") {
			// normal
			Normal normal;
			iss >> normal.nx >> normal.ny >> normal.nz;
			normals.push_back(normal);
		}
		else if (cmd == "f") {
			// face, one indexing in OBJ files
			Face face;
			char slash;

			for (int i = 0; i < 3; ++i) {
				iss >> face.vIdx[i] >> slash >> face.tIdx[i] >> slash >> face.nIdx[i];
				// convert to zero indexing
				face.vIdx[i]--;
				face.tIdx[i]--;
				face.nIdx[i]--;
			}
			faces.push_back(face);
		}
	}

	objFile.close();
	return true;
}

// setup OpenGL buffers
void OBJLoader::setupBuffers() {
	std::vector<float> vertexData;
	std::vector<unsigned int> indices;

	// setup vertex data
	for (const auto& face : faces) {
		for (int i = 0; i < 3; ++i) {
			Vertex v = vertices[face.vIdx[i]];
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

			indices.push_back(indices.size());
		}
	}

	// generate and bind VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// generate VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// normal attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// generate EBO
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// unbind VAO
	glBindVertexArray(0);
}

void OBJLoader::renderModel() {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// load materials from file path
bool OBJLoader::loadMaterials(const std::string& mtlPath) {
	std::ifstream file(mtlPath);

	if (!file.is_open()) {
		std::cerr << "Failed to open " << mtlPath << std::endl;
		return false;
	}
	std::cout << "Loaded " << mtlPath << std::endl;

	std::string line;
	Material currentMaterial;

	while (std::getline(file, line)) {
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
	}

	if (!currentMaterial.name.empty()) {
		materials[currentMaterial.name] = currentMaterial;
	}
	return true;
}

// load texture in OpenGL
GLuint OBJLoader::loadTexture(const std::string& texturePath) {
	GLuint textureID;
	glGenTextures(1, &textureID);

	// load image data
	int width = 0;
	int height = 0;
	int channelCount = 0;
	unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channelCount, 0);

	if (data) {
		std::cout << "Loaded " << texturePath << std::endl;

		glBindTexture(GL_TEXTURE_2D, textureID);

		// set texture wrapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// set texture filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// determine texture format
		GLenum fmt = (channelCount == 1) ? GL_RED : (channelCount == 3) ? GL_RGB : GL_RGBA;

		// load texture data
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "Failed to load texture: " << texturePath << std::endl;
	}

	// clean up
	stbi_image_free(data);

	return textureID;
}