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
	
		void Init(RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, VertexLayout* pVertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace);

	private:
		VulkanDevice*			m_pDevice = nullptr;

		VkPipelineLayout		m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline				m_Pipeline = VK_NULL_HANDLE;
	};
}

