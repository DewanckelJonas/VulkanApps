#pragma once
#include "Platform.h"
#include <string>

namespace vkw
{
	class VulkanDevice;
	class RenderPass;
	class VertexLayout;
	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(VulkanDevice* pDevice, RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, VertexLayout* pVertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace);
		~GraphicsPipeline();
		VkPipelineLayout GetLayout();
		VkPipeline GetPipeline();

	private:
		void Init(RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, VertexLayout* pVertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace);
		void Cleanup();
		VulkanDevice*			m_pDevice = nullptr;

		VkPipelineLayout		m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline				m_Pipeline = VK_NULL_HANDLE;
	};
}

