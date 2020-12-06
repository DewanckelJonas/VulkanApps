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
	//TODO: DEALLOCATE ALLL SETS AT ONCE FOR PERFORMANCE REASONS.
	for (vkw::DescriptorSet* descriptorSet : m_pDescriptorSets)
	{
		descriptorSet->DeAllocate(m_pDevice);
		delete descriptorSet;
	}
	vkDestroyDescriptorPool(m_pDevice->GetDevice(), m_Pool, nullptr);
}

void vkw::DescriptorPool::AddDescriptorSet(vkw::DescriptorSet * pDescriptorSet)
{
	m_pDescriptorSets.push_back(pDescriptorSet);
	for(const VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding : pDescriptorSet->GetDescriptorSetLayoutBindings())
	{
		auto it = std::find_if(m_DescriptorPoolSizes.begin(), m_DescriptorPoolSizes.end(),
			[descriptorSetLayoutBinding](const VkDescriptorPoolSize& value)
			{ return descriptorSetLayoutBinding.descriptorType == value.type; });
		if(it != m_DescriptorPoolSizes.end())
		{
			it->descriptorCount += descriptorSetLayoutBinding.descriptorCount;
		}
		else
		{
			VkDescriptorPoolSize descriptorPoolSize{};
			descriptorPoolSize.type = descriptorSetLayoutBinding.descriptorType;
			descriptorPoolSize.descriptorCount = descriptorSetLayoutBinding.descriptorCount;
			m_DescriptorPoolSizes.push_back(descriptorPoolSize);
		}
	}
	m_NrOfSets++;
}

void vkw::DescriptorPool::Allocate()
{
	m_DescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	m_DescriptorPoolInfo.poolSizeCount = uint32_t(m_DescriptorPoolSizes.size());
	m_DescriptorPoolInfo.pPoolSizes = m_DescriptorPoolSizes.data();
	m_DescriptorPoolInfo.maxSets = m_NrOfSets;

	ErrorCheck(vkCreateDescriptorPool(m_pDevice->GetDevice(), &m_DescriptorPoolInfo, nullptr, &m_Pool));

	//TODO: ALLOCATE ALLL SETS AT ONCE FOR PERFORMANCE REASONS.
	for(vkw::DescriptorSet* descriptorSet : m_pDescriptorSets)
	{
		descriptorSet->Allocate(m_pDevice, this);
	}
}

VkDescriptorPool vkw::DescriptorPool::GetHandle()
{
	return m_Pool;
}
