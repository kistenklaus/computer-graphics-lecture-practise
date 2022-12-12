#include <cglib/colors/font.h>

#include <fstream>
#include <iostream>

static bool loadBinaryModel(const char* path, std::vector<glm::vec3>& vertices, std::vector<glm::uvec3>& indices) {
	static_assert(sizeof(glm::vec3) == 3 * sizeof(float), "unexpected size of glm::vec3!");

	std::ifstream f(path, std::ios::in | std::ios::binary);

	if (!f.good())
		return false;

	uint32_t countVertices;
	uint32_t countIndices;

	f.read(reinterpret_cast<char*>(&countVertices), 4);
	f.read(reinterpret_cast<char*>(&countIndices), 4);

	const size_t SIZE_VERTICES = countVertices * 3 * sizeof(float);
	const size_t SIZE_INDICES = countIndices * sizeof(glm::uvec3);

	vertices.resize(countVertices);
	indices.resize(countIndices);

	f.read(reinterpret_cast<char*>(vertices.data()), countVertices * 3 * sizeof(float));
	f.read(reinterpret_cast<char*>(indices.data()), countIndices * sizeof(glm::uvec3));

	return f.tellg() == std::streampos(8 + SIZE_VERTICES + SIZE_INDICES);
}

void build_font_triangles(std::vector<glm::vec3>& c_vertices, std::vector<glm::vec3>& g_vertices) {
	std::vector<glm::vec3> vertices;
	std::vector<glm::uvec3> indices;

	if (loadBinaryModel("assets/c.bin", vertices, indices)) {
		c_vertices.resize(indices.size() * 3);

		size_t offset = 0;
		for (glm::uvec3& in : indices) {
			c_vertices[offset + 0] = vertices.at(in.x);
			c_vertices[offset + 1] = vertices.at(in.y);
			c_vertices[offset + 2] = vertices.at(in.z);

			offset += 3;
		}
	}
	else {
		std::cerr << "failed to load assets/c.bin" << std::endl;
	}

	if (loadBinaryModel("assets/g.bin", vertices, indices)) {
		g_vertices.resize(indices.size() * 3);

		size_t offset = 0;
		for (glm::uvec3& in : indices) {
			g_vertices[offset + 0] = vertices.at(in.x);
			g_vertices[offset + 1] = vertices.at(in.y);
			g_vertices[offset + 2] = vertices.at(in.z);

			offset += 3;
		}
	}
	else {
		std::cerr << "failed to load assets/d.bin" << std::endl;
	}
}
