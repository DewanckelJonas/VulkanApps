#pragma once
#include "Platform.h"
#include <string>

namespace vkw
{
	class VulkanDevice;
	class RenderPass;
	class ComputePipeline
	{
	public:
		ComputePipeline(VulkanDevice* pDevice, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, const std::string& computeShader);
		~ComputePipeline();
		VkPipelineLayout GetLayout();
		VkPipeline GetPipeline();
		void Rebuild();

	private:
		void Init();
		void Cleanup();
		VulkanDevice* m_pDevice = nullptr;

		VkPipelineLayout			m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline					m_Pipeline = VK_NULL_HANDLE;

		RenderPass* m_pRenderPass = nullptr;
		VkPipelineCache				m_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorSetLayout		m_DescriptorSetLayout = VK_NULL_HANDLE;
		std::string					m_ComputeShaderPath{};
	};
}