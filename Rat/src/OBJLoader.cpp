// parse and load embedded OBJ file

#include <iostream>
#include <sstream>
#include <windows.h>

#include "OBJLoader.h"
#include "debug.h"
#include "resource.h"

OBJLoader::OBJLoader() : VAOs(0), VBO(0), EBO(0), textureID(0), materials{} {
	// nop
}

OBJLoader::~OBJLoader() {
	for (int i = 0; i < VAOs.size(); i++) {
		glDeleteVertexArrays(1, &VAOs[i]);
	}
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteTextures(1, &textureID);
}

// set resource mapping for embedded files
void OBJLoader::setResourceMapping(std::map<std::string, int> rm) {
	resourceMapping = rm;
}

// load embedded resource using rc file and resource header definitions
std::stringstream OBJLoader::loadEmbeddedResource(const std::string& fileName) {
	
	// check if mapping existings for embedded file
	if (!resourceMapping.count(fileName)) {
		DEBUG_STDERR("Could not find resource mapping for \"" << fileName << "\"" << std::endl);
		std::exit(-1);
	}

	// get resource ID from embedded file name
	const int resourceId = resourceMapping[fileName];
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
bool OBJLoader::parseObj(std::stringstream& objStream) {
	std::string line;
	int lineNum = 1;
	std::string matName;

	// parse OBJ commands
	while (std::getline(objStream, line)) {
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		if (cmd == "mtllib") {
			std::string mtlFileName;
			iss >> mtlFileName;

			if (!loadEmbeddedMtl(mtlFileName)) {
				DEBUG_STDERR("ERROR: MTL file " << mtlFileName << " failed to parse" << std::endl);
				return false;
			}
		}
		else if (cmd == "usemtl") {
			iss >> matName;

			// load texture for material if exists
			if (materials.count(matName) > 0) {
				if (!loadEmbeddedTexture(materials[matName].texturePath)) {
					DEBUG_STDERR("ERROR: Texture " << materials[matName].texturePath << " failed to parse" << std::endl);
					return false;
				}
			}
		}
		else if (cmd == "v") {
			VertexPos vertex{};
			iss >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (cmd == "vt") {
			TexCoord texCoord{};
			iss >> texCoord.u >> texCoord.v; // UV coordinates
			texCoord.v = 1.0f - texCoord.v; // flip, OBJ's UV map seemed to export upside down
			texCoords.push_back(texCoord);
		}
		else if (cmd == "vn") {
			Normal normal{};
			iss >> normal.nx >> normal.ny >> normal.nz; // normals
			normals.push_back(normal);
		}
		else if (cmd == "f") {
			Face face{};
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
			DEBUG_STDOUT("WARN: Unsupported command \"" << cmd << "\" on line " << lineNum << std::endl);
		}
		lineNum++;
	}
	return true;
}

// parse MTL from stream
bool OBJLoader::parseMtl(std::stringstream& mtlStream) {
	std::string line;
	int lineNum = 1;
	Material mat;

	// parse MTL commands
	while (std::getline(mtlStream, line)) {
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		if (cmd == "newmtl") {
			if (!mat.name.empty()) {
				materials[mat.name] = mat;
			}
			iss >> mat.name;
		}
		else if (cmd == "map_Kd") {
			iss >> mat.texturePath; // diffuse map

			// check if texture path is *.bmp
			if (!std::equal(mat.texturePath.begin() + mat.texturePath.size() - 4, mat.texturePath.end(), ".bmp")) {
				DEBUG_STDERR("ERROR: Cannot load " << mat.texturePath << ". Only .bmp textures are supported" << std::endl);
				return false;
			}
		}
		else if (cmd == "#" || cmd == " " || cmd == "") {
			// comment or empty line, do nothing
		}
		else {
			DEBUG_STDOUT("WARN: Unsupported command \"" << cmd << "\" on line " << lineNum << std::endl);
		}
		lineNum++;
	}

	// set current material
	if (!mat.name.empty()) {
		materials[mat.name] = mat;
	}
	return true;
}

// parse a 4-bit BMP from stream
uint8_t* OBJLoader::parseBmp4(std::stringstream& bmpStream, int& width, int& height, int& channels) {

	BITMAPFILEHEADER fileHeader{};
	bmpStream.read(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER));

	// check BMP magic number ("BM" in little endian)
	if (fileHeader.bfType != 0x4D42) {
		DEBUG_STDERR("ERROR: Data in stream is not a BMP file" << std::endl);
		return nullptr;
	}

	BITMAPINFOHEADER infoHeader{};
	bmpStream.read(reinterpret_cast<char*>(&infoHeader), sizeof(BITMAPINFOHEADER));

	// check if 4-bit BMP
	if (infoHeader.biBitCount != 4) {
		DEBUG_STDERR("ERROR: Only 4-bit BMP files are supported. Found " << infoHeader.biBitCount << "-bit BMP" << std::endl);
		return nullptr;
	}

	width = infoHeader.biWidth;
	height = infoHeader.biHeight;
	channels = 3; // RGB

	// get color palette; 4-bit BMP is 16 (2^4) colors, each is 4 bytes
	const int paletteSize = 16;
	RGBQUAD colorTable[paletteSize]{}; // RGB + reserved (4 bytes)
	bmpStream.read(reinterpret_cast<char*>(colorTable), paletteSize * sizeof(RGBQUAD));

	// find pixel data size
	int rowSize = ((width + 1) / 2 + 3) & ~3; // row size padded to multiple of 4 bytes
	size_t dataSize = static_cast<size_t>(rowSize) * height;
	uint8_t* pixelData = new uint8_t[dataSize];

	// move to pixel data start and start reading
	bmpStream.seekg(fileHeader.bfOffBits, std::ios::beg);
	bmpStream.read(reinterpret_cast<char*>(pixelData), dataSize);
	uint8_t* imageData = new uint8_t[width * height * channels];

	// read pixel data
	for (int y = 0; y < height; y++) {
		int flippedY = height - y - 1; // vertical flip BMP data, BMP stores pixels bottom to top

		for (int x = 0; x < width; x += 2) {

			// each byte has two 4-bit pixels, one in each nibble
			uint8_t b = pixelData[flippedY * rowSize + x / 2];

			// first pixel in high nibble
			uint8_t high = (b >> 4) & 0x0F;
			imageData[(y * width + x) * 3 + 0] = colorTable[high].rgbRed;
			imageData[(y * width + x) * 3 + 1] = colorTable[high].rgbGreen;
			imageData[(y * width + x) * 3 + 2] = colorTable[high].rgbBlue;

			// second pixel in low nibble, if within bounds
			if (x + 1 < width) {
				uint8_t low = b & 0x0F;
				imageData[(y * width + (x + 1)) * 3 + 0] = colorTable[low].rgbRed;
				imageData[(y * width + (x + 1)) * 3 + 1] = colorTable[low].rgbGreen;
				imageData[(y * width + (x + 1)) * 3 + 2] = colorTable[low].rgbBlue;
			}
		}
	}
	delete[] pixelData;

	return imageData;
}

// set texture from stream
GLuint OBJLoader::createTexture(const std::string& texturePath, uint8_t* imageData, int width, int height, int channels) {
	glGenTextures(1, &textureID);

	if (imageData) {
		DEBUG_STDOUT("Loaded texture " << texturePath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl);
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
		DEBUG_STDERR("ERROR: Failed to load texture " << std::endl);
	}
	return textureID;
}

// load embedded OBJ model with file name
bool OBJLoader::loadEmbeddedObj(const std::string& fileName) {
	std::stringstream objStream = loadEmbeddedResource(fileName);
	DEBUG_STDOUT("Loaded OBJ " << fileName << std::endl);
	return parseObj(objStream);
}

// load embedded MTL with file name
bool OBJLoader::loadEmbeddedMtl(const std::string& fileName) {
	std::stringstream mtlStream = loadEmbeddedResource(fileName);
	DEBUG_STDOUT("Loaded MTL " << fileName << std::endl);
	return parseMtl(mtlStream);
}

// load embedded texture with file name
bool OBJLoader::loadEmbeddedTexture(const std::string& fileName) {

	// parse BMP and create texture from it
	std::stringstream bmpStream = loadEmbeddedResource(fileName);
	int width = 0, height = 0, channels = 0;
	uint8_t* imageData = parseBmp4(bmpStream, width, height, channels);

	if (!imageData) {
		DEBUG_STDERR("ERROR: Failed to parse BMP " << fileName << std::endl);
		return false;
	}
	textureID = createTexture(fileName, imageData, width, height, channels);

	// cleanup
	delete[] imageData;

	return true;
}

// setup OpenGL buffers
void OBJLoader::setupBuffers(int contextIdx) {
	DEBUG_STDOUT("Setting up buffers for context " << contextIdx << std::endl);

	std::vector<float> vertexData;
	std::vector<GLsizei> indices;

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
