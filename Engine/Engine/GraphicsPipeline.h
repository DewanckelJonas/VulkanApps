#pragma once
#include "Platform.h"
namespace vkw
{
	class VulkanDevice;
	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(VulkanDevice* pDevice, VkDescriptorSetLayout descriptorSetLayout);
		~GraphicsPipeline();
	
		void Init();

	private:
		VulkanDevice*			m_pDevice = nullptr;

		VkPipelineLayout		m_PipelineLayout = VK_NULL_HANDLE;
	};
}

