#include "VertexLayout.h"
#include <cassert>

vkw::VertexLayout::VertexLayout(const std::vector<VertexAttribute>& layout)
:m_Layout(layout)
{
	for (const VertexAttribute& type : m_Layout)
	{
		m_Stride += uint32_t(GetVertexTypeSize(type));
	}
}

uint32_t vkw::VertexLayout::GetStride()
{
	return m_Stride; 
}

const std::vector<VertexAttribute>& vkw::VertexLayout::GetLayout() 
{
	return m_Layout; 
}

const VkPipelineVertexInputStateCreateInfo& vkw::VertexLayout::CreateVertexDescription()
{
	m_BindingDescriptions[0].binding = 0;
	m_BindingDescriptions[0].stride = m_Stride;
	m_BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Attribute descriptions
	uint32_t offset{};
	m_AttributeDescriptions.resize(m_Layout.size());

	for (size_t i = 0; i < m_Layout.size(); i++)
	{
		uint32_t typeSize = uint32_t(GetVertexTypeSize(m_Layout[i]));
		if(typeSize == 1*sizeof(float))
		{
			m_AttributeDescriptions[i].format = VK_FORMAT_R32_SFLOAT;
		}
		else 
		if(typeSize == 2*sizeof(float))
		{
			m_AttributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
		}
		else
		if (typeSize == 3*sizeof(float))
		{
			m_AttributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
		}
		else
		if (typeSize == 4*sizeof(float))
		{
			m_AttributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;

		}else
		{
			assert(0 && "Unsupported vertextype size! Supported vertextype sizes are 1, 2, 3 and 4!");
		}
		m_AttributeDescriptions[i].binding = 0;
		m_AttributeDescriptions[i].location = uint32_t(i);
		m_AttributeDescriptions[i].offset = offset;
		offset += typeSize;
	}

	m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_VertexInputState.pVertexAttributeDescriptions = m_AttributeDescriptions.data();
	m_VertexInputState.vertexAttributeDescriptionCount = uint32_t(m_AttributeDescriptions.size());
	m_VertexInputState.pVertexBindingDescriptions = m_BindingDescriptions.data();
	m_VertexInputState.vertexBindingDescriptionCount = uint32_t(m_BindingDescriptions.size());
	return m_VertexInputState;
}