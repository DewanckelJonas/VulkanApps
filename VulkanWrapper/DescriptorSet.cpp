#include "DescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include "DescriptorPool.h"

using namespace vkw;
DescriptorSet::DescriptorSet()
{
}


DescriptorSet::~DescriptorSet()
{
}

void DescriptorSet::AddBinding(const VkDescriptorImageInfo & imageDescriptor, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage)
{
	uint32_t binding = uint32_t(m_WriteDescriptorSets.size());
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.dstBinding = binding;
	//dstSet gets set when allocating pool and creating the descriptorSet.
	writeDescriptorSet.dstSet = VK_NULL_HANDLE;
	writeDescriptorSet.pImageInfo = &imageDescriptor;
	writeDescriptorSet.descriptorCount = 1;
	m_WriteDescriptorSets.push_back(writeDescriptorSet);

	AddDescriptorSetLayoutBinding(descriptorType, shaderStage, binding);
}

void DescriptorSet::AddBinding(const VkDescriptorBufferInfo & bufferDescriptor, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage)
{
	uint32_t binding = uint32_t(m_WriteDescriptorSets.size());
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.dstBinding = binding;
	//dstSet gets set when allocating pool and creating the descriptorSet.
	writeDescriptorSet.dstSet = VK_NULL_HANDLE;
	writeDescriptorSet.pBufferInfo = &bufferDescriptor;
	writeDescriptorSet.descriptorCount = 1;
	m_WriteDescriptorSets.push_back(writeDescriptorSet);
	
	AddDescriptorSetLayoutBinding(descriptorType, shaderStage, binding);
}

void vkw::DescriptorSet::Allocate(VulkanDevice * pDevice, DescriptorPool * pPool)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = uint32_t(m_DescriptorSetLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = m_DescriptorSetLayoutBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(pDevice->GetDevice(), &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout));

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pPool->GetHandle();
	allocInfo.pSetLayouts = &m_DescriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	ErrorCheck(vkAllocateDescriptorSets(pDevice->GetDevice(), &allocInfo, &m_DescriptorSet));
	vkQueueWaitIdle(pDevice->GetQueue());


	for (VkWriteDescriptorSet& writeDescriptorSet : m_WriteDescriptorSets)
	{
		writeDescriptorSet.dstSet = m_DescriptorSet;
	}

	vkUpdateDescriptorSets(pDevice->GetDevice(), static_cast<uint32_t>(m_WriteDescriptorSets.size()), m_WriteDescriptorSets.data(), 0, nullptr);
	vkQueueWaitIdle(pDevice->GetQueue());

}

void vkw::DescriptorSet::DeAllocate(VulkanDevice* pDevice)
{
	vkDestroyDescriptorSetLayout(pDevice->GetDevice(), m_DescriptorSetLayout, nullptr);
}

const std::vector<VkDescriptorSetLayoutBinding>& vkw::DescriptorSet::GetDescriptorSetLayoutBindings() const
{
	return m_DescriptorSetLayoutBindings;
}

const VkDescriptorSet& vkw::DescriptorSet::GetHandle() const
{
	return m_DescriptorSet;
}

const VkDescriptorSetLayout& vkw::DescriptorSet::GetLayout() const
{
	return m_DescriptorSetLayout;
}

void vkw::DescriptorSet::AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, uint32_t binding)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings{};
	descriptorSetLayoutBindings.binding = binding;
	descriptorSetLayoutBindings.descriptorCount = 1;
	descriptorSetLayoutBindings.descriptorType = descriptorType;
	descriptorSetLayoutBindings.stageFlags = shaderStage;
	m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBindings);
}
