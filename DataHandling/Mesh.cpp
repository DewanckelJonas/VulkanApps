#include "Mesh.h"
#include <algorithm>
#include <iostream>

std::vector<float> Mesh::CreateVertices(const std::vector<VertexAttribute>& vertexTypes, const glm::mat4x4& transform)
{
	size_t length = GetVertexDataSize(vertexTypes)/sizeof(float);
	std::vector<float> vertices(length);
	CreateVertices(vertexTypes, vertices.data(), transform);
	return vertices;
}

float* Mesh::CreateVertices(const std::vector<VertexAttribute>& vertexTypes, float* allocatedMem, const glm::mat4x4& transform)
{
	std::map<VertexAttribute, size_t> vertexAttributeCounts;
	size_t vertexCount = GetVertexAttributeCount(vertexTypes.front());
	size_t stride = GetStride(vertexTypes);
	size_t offset{};
	for (VertexAttribute attributeType : vertexTypes)
	{
		float* writePos = allocatedMem + offset;
		size_t vertexAttributeCount = GetVertexAttributeCount(attributeType);
		size_t vertexAttributeSize = GetVertexTypeSize(attributeType);
		for (size_t i = 0; i < vertexAttributeCount; i++)
		{
			WriteVertexAttribute(attributeType, i, writePos, transform);
			writePos += stride/sizeof(float);
		}
		for (size_t i = vertexAttributeCount; i < vertexCount; i++)
		{
			if (m_ShouldFillVertexAttributes.end() == std::find(m_ShouldFillVertexAttributes.begin(), m_ShouldFillVertexAttributes.end(), attributeType))
			{
				assert(0 && "VertexAttribute length smaller then vertexcount! Set ShoulFillVertexAttribute=true if this was intentional");
			}
			WriteVertexAttribute(attributeType, vertexAttributeCount - 1, writePos, transform);
			writePos += stride/sizeof(float);
		}
		offset += vertexAttributeSize/sizeof(float);
	}
	return allocatedMem + vertexCount*(stride/sizeof(float));
}

void Mesh::SetIndices(const std::vector<uint32_t>& indices)
{
	m_Indices = indices;
}

size_t Mesh::GetVertexDataSize(const std::vector<VertexAttribute>& vertexTypes)
{
	size_t vertexCount = GetVertexAttributeCount(vertexTypes.front());
	size_t size{};
	for (VertexAttribute type : vertexTypes)
	{
		size += vertexCount * GetVertexTypeSize(type);
	}
	return size;
}


const std::vector<uint32_t>& Mesh::GetIndices()
{
	return m_Indices;
}

uint32_t* Mesh::GetIndices(uint32_t vertexOffset, uint32_t* allocatedMem)
{
	std::vector<uint32_t> offsetIndices = m_Indices;
	for (uint32_t& index : offsetIndices)
	{
		index += vertexOffset;
	}
	memcpy(allocatedMem, offsetIndices.data(), offsetIndices.size() * sizeof(uint32_t));
	return allocatedMem + offsetIndices.size();
}

size_t Mesh::GetVertexCount(const std::vector<VertexAttribute>& attributeTypes)
{
	return GetVertexAttributeCount(attributeTypes.front());
}

size_t Mesh::GetIndexCount()
{
	return m_Indices.size();
}


void Mesh::WriteVertexAttribute(VertexAttribute attributeType, size_t idx, float* writePos, const glm::mat4x4& transform)
{
	size_t attributeSize = GetVertexTypeSize(attributeType);
	idx *= (attributeSize / sizeof(float));

	memcpy(writePos, &m_VertexAttributes[attributeType][idx], attributeSize);
	if(IsAffectedByTransform(attributeType) && transform != glm::mat4x4(1.f))
	{
		glm::vec4 attribute{};
		assert(attributeSize > 2*sizeof(float) && "Attribute size is not supported for transform but is tagged as IsAffectedByTransform");
		if(attributeSize == 3*sizeof(float))
		{
			attribute = glm::vec4(*(glm::vec3*)writePos, 1.f);
		}else
		if(attributeSize == 4*sizeof(float))
		{
			attribute = *(glm::vec4*)writePos;
		}
		attribute = transform * attribute;
		memcpy(writePos, &attribute, attributeSize);
	}
}



void Mesh::SetFillVertexAttribute(VertexAttribute attributeType, bool shouldFill)
{
	if(shouldFill)
	{
		if (m_ShouldFillVertexAttributes.end() == std::find(m_ShouldFillVertexAttributes.begin(), m_ShouldFillVertexAttributes.end(), attributeType))
		{
			m_ShouldFillVertexAttributes.push_back(attributeType);
		}
	}
	else
	{
		m_ShouldFillVertexAttributes.erase(std::remove(m_ShouldFillVertexAttributes.begin(), m_ShouldFillVertexAttributes.end(), attributeType), m_ShouldFillVertexAttributes.end());
	}
}


size_t Mesh::GetVertexAttributeCount(VertexAttribute attributeType)
{
	size_t attributeSize = GetVertexTypeSize(attributeType);

	auto attributesIt = m_VertexAttributes.find(attributeType);
	if (attributesIt != m_VertexAttributes.end())
	{
		return attributesIt->second.size()/(attributeSize/sizeof(float));
	}
	return 0;
}
