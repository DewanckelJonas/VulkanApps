#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <Base/VertexTypes.h>
#include <map>
class Mesh
{
public:
	Mesh() {};

	//Usefull for batching multiple meshes into one vertexBuffer returns the next writepos;
	float* CreateVertices(const std::vector<VertexAttribute>& vertexTypes, float* allocatedMem, const glm::mat4x4& transform = glm::mat4x4(1.f));
	std::vector<float> CreateVertices(const std::vector<VertexAttribute>& vertexTypes, const glm::mat4x4& transform = glm::mat4x4(1.f));

	//Usefull for batching all indices into one indexBuffer returns the next writepos;
	uint32_t* GetIndices(uint32_t vertexOffset, uint32_t* allocatedMem);
	const std::vector<uint32_t>& GetIndices();

	//Vertex count is based on the first attribute
	size_t GetVertexCount(const std::vector<VertexAttribute>& attributeTypes);
	size_t GetIndexCount();
	size_t GetVertexDataSize(const std::vector<VertexAttribute>& attributeTypes);

	void SetIndices(const std::vector<uint32_t>& indices);
	template<typename T>
	void AddVertexAttribute(VertexAttribute type, const std::vector<T>& data)
	{
		assert(GetVertexTypeSize(type) == sizeof(T) && "Vertex Attribute type does not have the same size as attributes passed!");
		//Copies vector of any type into float vector
		float* front = (float*)(&data[0]);
		float* back = (float*)(&data[data.size()-1])+(sizeof(T)/sizeof(float));
		m_VertexAttributes[type] = std::vector<float>(front, back);
	}

	//If enabled this will fill vertex attribute data to match the length of the first vertexattribute with the last value passed in the vertexattributes data if no data was passed 0 initialized.
	void SetFillVertexAttribute(VertexAttribute attributeType, bool shouldFill);
	
private:
	size_t GetVertexAttributeCount(VertexAttribute attributeType);
	void WriteVertexAttribute(VertexAttribute attributeType, size_t idx, float* writePos, const glm::mat4x4& transform = glm::mat4x4(1.f));

	std::vector<uint32_t>									m_Indices{};
	std::map<VertexAttribute, std::vector<float>>			m_VertexAttributes{};

	std::vector<VertexAttribute>							m_ShouldFillVertexAttributes{};
};
