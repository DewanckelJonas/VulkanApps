#pragma once
#include <Base/Array3D.h>
#include <stdint.h>
#include <DataHandling/Mesh.h>
#include <glm/glm.hpp>
#include <Base/Ray.h>
class VoxelChunk
{
public:
	VoxelChunk(const Array3D<uint32_t>& data, const glm::vec3& position = {0,0,0});
	const Mesh& GetMesh() const { return m_Mesh; }
	void GenerateMesh();
	Array3D<uint32_t>& GetData() { return m_VoxelData; };
	bool Raycast(glm::ivec3& id, const Ray& ray, float minDist = 0, float maxDist = FLT_MAX);
	const std::vector<float>& GetVertexBuffer() { return m_Vertices; }
	const std::vector<uint32_t>& GetIndexBuffer() { return m_Indices; }
	const glm::ivec3& GetVoxel(glm::vec3 position);
	bool IsInChunk(glm::vec3 pos) const;

private:
	Array3D<uint32_t> m_VoxelData;
	Mesh m_Mesh;
	std::vector<float> m_Vertices{};
	std::vector<uint32_t> m_Indices{};
	glm::vec3 m_Position{};
};

