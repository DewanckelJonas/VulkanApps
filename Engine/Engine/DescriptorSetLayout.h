#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout();
		void AddBinding(VkDescriptorType type, VkShaderStageFlags shaderStage);
		size_t GetSize();
		const std::vector<VkDescriptorSetLayoutBinding>& GetRaw();
	private:
		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayouts{};
	};
}

