#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class VulkanDevice;
	class DescriptorSet;
	class DescriptorPool
	{
	public:
		DescriptorPool(VulkanDevice* pDevice);
		~DescriptorPool();

		void AddDescriptorSet(DescriptorSet* pDescriptorSet);
		void Allocate();
		VkDescriptorPool GetHandle();

	private:
		VulkanDevice*							m_pDevice = nullptr;
		VkDescriptorPool						m_Pool = VK_NULL_HANDLE;
		VkDescriptorPoolCreateInfo				m_DescriptorPoolInfo{};
		uint32_t								m_NrOfSets = 0;
		std::vector<VkDescriptorPoolSize>		m_DescriptorPoolSizes{};
		std::vector<DescriptorSet*>			m_pDescriptorSets{};
	};
}
