#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class VertexLayout
	{
	public:

		enum class Types
		{
			POSITION,
			NORMAL,
			COLOR,
			UV,
			TANGENT,
			BITANGENT,
			PADDINGFLOAT,
			PADDINGVEC4
		};

		VertexLayout(const std::vector<Types>& layout)
			:m_Layout(layout)
		{
			for (const Types& type : m_Layout)
			{
				switch (type)
				{
				case Types::UV:
					m_Stride += 2 * sizeof(float);
					break;
				case Types::PADDINGFLOAT:
					m_Stride += sizeof(float);
					break;
				case Types::PADDINGVEC4:
					m_Stride += 4 * sizeof(float);
					break;
				default:
					m_Stride += 3 * sizeof(float);
				}
			}
		}

		uint32_t GetStride() { return m_Stride; }
		const std::vector<Types>& GetLayout() { return m_Layout; }
		const VkPipelineVertexInputStateCreateInfo& CreateVertexDescription();

	private:
		uint32_t m_Stride{};
		std::vector<Types> m_Layout{};
		std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions{};
		std::vector<VkVertexInputBindingDescription> m_BindingDescriptions{ 1 };
		VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
	};
}

