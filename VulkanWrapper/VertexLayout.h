#pragma once
#include "Platform.h"
#include <vector>
#pragma once
#include <Base/VertexTypes.h>
namespace vkw
{
	class VertexLayout
	{
	public:

		VertexLayout(const std::vector<VertexAttribute>& layout);

		uint32_t GetStride();
		const std::vector<VertexAttribute>& GetLayout();
		const VkPipelineVertexInputStateCreateInfo& CreateVertexDescription();

	private:
		uint32_t m_Stride{};
		std::vector<VertexAttribute> m_Layout{};
		std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions{};
		std::vector<VkVertexInputBindingDescription> m_BindingDescriptions{ 1 };
		VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
	};
}

