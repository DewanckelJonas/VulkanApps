#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include <algorithm>
#include "VulkanHelpers.h"
#include "VulkanDevice.h"

vkw::DescriptorPool::DescriptorPool(VulkanDevice * pDevice)
	:m_pDevice(pDevice)
{
}

vkw::DescriptorPool::~DescriptorPool()
{
}

void vkw::DescriptorPool::AddDescriptorSet(DescriptorSet * pDescriptorSet)
{
	for(const VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding : pDescriptorSet->GetDescriptorSetLayoutBindings())
	{
		auto it = std::find_if(m_DescriptorPoolSizes.begin(), m_DescriptorPoolSizes.end(),
			[descriptorSetLayoutBinding](const VkDescriptorPoolSize& value)
			{ return descriptorSetLayoutBinding.descriptorType == value.type; });
		if(it != m_DescriptorPoolSizes.end())
		{
			++(it->descriptorCount);
		}
		else
		{
			VkDescriptorPoolSize setLayoutBinding{};
			setLayoutBinding.type = descriptorSetLayoutBinding.descriptorType;
			setLayoutBinding.descriptorCount = 1;
			m_DescriptorPoolSizes.push_back(setLayoutBinding);
		}
	}
	m_NrOfSets++;
}

void vkw::DescriptorPool::Allocate()
{
	m_DescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	m_DescriptorPoolInfo.poolSizeCount = m_DescriptorPoolSizes.size();
	m_DescriptorPoolInfo.pPoolSizes = m_DescriptorPoolSizes.data();
	m_DescriptorPoolInfo.maxSets = m_NrOfSets;

	ErrorCheck(vkCreateDescriptorPool(m_pDevice->GetDevice(), &m_DescriptorPoolInfo, nullptr, &m_Pool));
}

VkDescriptorPool vkw::DescriptorPool::GetHandle()
{
	return m_Pool;
}
