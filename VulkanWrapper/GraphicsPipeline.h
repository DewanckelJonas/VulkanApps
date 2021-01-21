#pragma once
#include "Platform.h"
#include <string>
#include "VertexLayout.h"

namespace vkw
{
	class VulkanDevice;
	class RenderPass;
	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(VulkanDevice* pDevice, RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, const VertexLayout& vertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE, bool wireFrame = false);
		~GraphicsPipeline();
		VkPipelineLayout GetLayout();
		VkPipeline GetPipeline();
		void Rebuild();

	private:
		void Init();
		void Cleanup();
		VulkanDevice*				m_pDevice = nullptr;

		VkPipelineLayout			m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline					m_Pipeline = VK_NULL_HANDLE;

		RenderPass*					m_pRenderPass = nullptr;
		VkPipelineCache				m_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorSetLayout		m_DescriptorSetLayout = VK_NULL_HANDLE;
		VertexLayout				m_VertexLayout;
		std::string                 m_VertexShaderPath{};
		std::string					m_FragmentShaderPath{};
		VkPrimitiveTopology			m_Topology{};
		VkFrontFace					m_FrontFace{};
		bool						m_Wireframe;
	};
}

