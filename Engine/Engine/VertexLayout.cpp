#include "VertexLayout.h"


VkPipelineVertexInputStateCreateInfo vkw::VertexLayout::CreateVertexDescription()
{
	m_BindingDescriptions[0].binding = 0;
	m_BindingDescriptions[0].stride = m_Stride;
	m_BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


	// Attribute descriptions

	uint32_t offset{};
	m_AttributeDescriptions.resize(m_Layout.size());
	for (size_t i = 0; i < m_Layout.size(); i++)
	{
		switch (m_Layout[i])
		{
		case Types::UV:
			m_AttributeDescriptions[i].binding = 0;
			m_AttributeDescriptions[i].location = i;
			m_AttributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
			m_AttributeDescriptions[i].offset = offset;
			offset += 2 * sizeof(float);
			break;
		case Types::PADDINGFLOAT:
			offset += sizeof(float);
			break;
		case Types::PADDINGVEC4:
			offset += 4 * sizeof(float);
			break;
		default:
			m_AttributeDescriptions[i].binding = 0;
			m_AttributeDescriptions[i].location = i;
			m_AttributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
			m_AttributeDescriptions[i].offset = offset;
			offset += 3 * sizeof(float);
		}
	}


	VkPipelineVertexInputStateCreateInfo vertexInputState{};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pVertexAttributeDescriptions = m_AttributeDescriptions.data();
	vertexInputState.vertexAttributeDescriptionCount = m_AttributeDescriptions.size();
	vertexInputState.pVertexBindingDescriptions = m_BindingDescriptions.data();
	vertexInputState.vertexBindingDescriptionCount = m_BindingDescriptions.size();
	return vertexInputState;
}