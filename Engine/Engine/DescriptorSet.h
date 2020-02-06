#pragma once
#include "Platform.h"
#include <vector>

namespace vkw
{
	class VulkanDevice;
	class DescriptorPool;
	class DescriptorSet
	{
	public:
		DescriptorSet();
		~DescriptorSet();

		void AddBinding(const VkDescriptorImageInfo& imageDescriptor, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage);
		void AddBinding(const VkDescriptorBufferInfo& bufferDescriptor, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage);

		void Allocate(VulkanDevice* pDevice ,DescriptorPool* pPool);

		const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetLayoutBindings() const;

		VkDescriptorSetLayout  GetLayout() const;

	private:
		VulkanDevice*									m_pDevice = nullptr;
		VkDescriptorSetLayout							m_DescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding>		m_DescriptorSetLayoutBindings{};
		VkDescriptorSet									m_DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetAllocateInfo						m_DescriptorSetInfo{};
		std::vector<VkWriteDescriptorSet>				m_WriteDescriptorSets;

		void AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStage);

	};
}

