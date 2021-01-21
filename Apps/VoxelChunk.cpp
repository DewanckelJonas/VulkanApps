#include "VoxelChunk.h"
#include <DataHandling/MeshShapes.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

VoxelChunk::VoxelChunk(const Array3D<uint32_t>& data, const glm::vec3& position)
	:m_VoxelData{data}
	,m_Position{position}
{
}

void VoxelChunk::GenerateMesh()
{
	Mesh* cubeMesh = CreateCubeMesh(1.f, { 0.f,0.f }, { 0.3f, 0.6f, 0.f, 1.f });
	std::vector<VertexAttribute> attributes = { VertexAttribute::POSITION, VertexAttribute::COLOR, VertexAttribute::NORMAL };
	size_t cubeDataSize = cubeMesh->GetVertexDataSize(attributes) / sizeof(float);

	const size_t size{ m_VoxelData.GetDepth() };
	m_Vertices.clear();
	m_Vertices.resize(size * size * size * cubeDataSize);
	float* writePos = m_Vertices.data();
	size_t vertexOffset = 0;
	m_Indices.clear();
	m_Indices.resize(cubeMesh->GetIndexCount() * size * size * size);
	uint32_t* indexWritePos = m_Indices.data();
	size_t vertexCount = 0;
	size_t indexCount = 0;
	for (size_t y = 0; y < size; y++)
	{
		for (size_t z = 0; z < size; z++)
		{
			for (size_t x = 0; x < size; x++)
			{
				if (m_VoxelData[x][y][z] != 0)
				{
					if (x != 0 && y != 0 && z != 0 && x != m_VoxelData.GetWidth()-1 && y != m_VoxelData.GetHeight()-1 && z != m_VoxelData.GetDepth()-1)
					{
						if (m_VoxelData[x + 1][y][z] != 0 && m_VoxelData[x][y + 1][z] != 0 && m_VoxelData[x][y][z + 1] != 0
							&& m_VoxelData[x - 1][y][z] != 0 && m_VoxelData[x][y - 1][z] != 0 && m_VoxelData[x][y][z - 1])
						{
						}
						else
						{
							vertexCount += cubeDataSize;
							indexCount += cubeMesh->GetIndexCount();
							glm::mat4x4 translation = glm::translate(glm::mat4x4(1.f), { x+m_Position.x+0.5f, y+m_Position.y+0.5f , z+m_Position.z+0.5f });
							writePos = cubeMesh->CreateVertices(attributes, writePos, translation);
							indexWritePos = cubeMesh->GetIndices(vertexOffset, indexWritePos);
							vertexOffset += cubeMesh->GetVertexCount(attributes);
						}
					}
					else
					{
						vertexCount += cubeDataSize;
						indexCount += cubeMesh->GetIndexCount();
						glm::mat4x4 translation = glm::translate(glm::mat4x4(1.f), { x+m_Position.x+0.5f, y+m_Position.y+0.5f , z+m_Position.z+0.5f });
						writePos = cubeMesh->CreateVertices(attributes, writePos, translation);
						indexWritePos = cubeMesh->GetIndices(vertexOffset, indexWritePos);
						vertexOffset += cubeMesh->GetVertexCount(attributes);
					}
				}
			}
		}
	}
	m_Vertices.resize(vertexCount);
	m_Indices.resize(indexCount);
}


bool VoxelChunk::Raycast(glm::ivec3& voxelId, const Ray& ray, float minDist, float maxDist)
{
	AABox box{ m_Position, {m_VoxelData.GetWidth(), m_VoxelData.GetHeight(), m_VoxelData.GetHeight()} };
	float tenter, texit;
	float toffset = 0.0001f; //How much farther the ray traverses through each block to make sure we never hit end on an edge.
	glm::vec3 pos;
	float t;
	if (!IsInChunk(ray.Pos))
	{
		if (!ray.Intersect(box, tenter, texit))
			return false;
		else
			t = tenter + toffset;
	}else
	{
		t = 0;
	}
	if (t > maxDist)
		return false;
	if (t < minDist)
		return false;
	pos = ray.Traverse(t);
	if (!IsInChunk(pos))
		return false;
	glm::ivec3 prevVoxel = GetVoxel(pos);
	glm::ivec3 currVoxel = GetVoxel(pos);
	
	/*for (size_t x = 0; x < m_VoxelData.GetWidth(); x++)
	{
		for (size_t y = 0; y < m_VoxelData.GetHeight(); y++)
		{
			for (size_t z = 0; z < m_VoxelData.GetDepth(); z++)
			{
				std::cout << "x: " << x << ", y: " << y << ", z: " << z << " Value: " << m_VoxelData[x][y][z] << std::endl;
			}
		}
	}*/
	const int maxSteps = 100.f;
	int steps = 0;
	while (m_VoxelData[currVoxel.x][currVoxel.y][currVoxel.z] == 0)
	{
		if (steps++ >= maxSteps)
			return false;
		AABox voxelBounds{ glm::vec3(currVoxel)+m_Position, {1.f, 1.f, 1.f} };
		if(ray.Intersect(voxelBounds, tenter, texit))
		{
			t = texit + toffset;
			if (t > maxDist)
				return false;
			if (t < minDist)
				return false;
		}else
		{
			return false;
		}
		glm::vec3 pos = ray.Traverse(t);
		currVoxel = GetVoxel(pos);
		if(!IsInChunk(pos))
		{
			return false;
		}
	}
	voxelId = currVoxel;
	return true;

}

const glm::ivec3& VoxelChunk::GetVoxel(glm::vec3 position)
{
	glm::ivec3 voxelId;
	voxelId.x = glm::min(glm::max(int(floor(position.x - m_Position.x)), 0), int(m_VoxelData.GetWidth()-1));
	voxelId.y = glm::min(glm::max(int(floor(position.y - m_Position.y)), 0), int(m_VoxelData.GetHeight()-1));
	voxelId.z = glm::min(glm::max(int(floor(position.z - m_Position.z)), 0), int(m_VoxelData.GetDepth()-1));
	return voxelId;
}

bool VoxelChunk::IsInChunk(glm::vec3 pos) const
{
	glm::vec3 transPos{ pos - m_Position };
	return !(transPos.x < 0 || transPos.y < 0 || transPos.z < 0
		|| transPos.x > float(m_VoxelData.GetWidth()) || transPos.y > float(m_VoxelData.GetHeight()) || transPos.z-FLT_EPSILON > float(m_VoxelData.GetDepth()));
}

	


