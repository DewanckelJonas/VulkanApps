#include "DescriptorSetLayout.h"

vkw::DescriptorSetLayout::DescriptorSetLayout()
{
}

void vkw::DescriptorSetLayout::AddBinding(VkDescriptorType type, VkShaderStageFlags shaderStage)
{
	VkDescriptorSetLayoutBinding descriptorSetLayout{};
	descriptorSetLayout.descriptorType = type;
	descriptorSetLayout.descriptorCount = 1;
	descriptorSetLayout.stageFlags = shaderStage;
	descriptorSetLayout.binding = uint32_t(m_DescriptorSetLayouts.size());
	m_DescriptorSetLayouts.push_back(descriptorSetLayout);
}

size_t vkw::DescriptorSetLayout::GetSize()
{
	return m_DescriptorSetLayouts.size();
}

const std::vector<VkDescriptorSetLayoutBinding>& vkw::DescriptorSetLayout::GetRaw()
{
	return m_DescriptorSetLayouts;
}
