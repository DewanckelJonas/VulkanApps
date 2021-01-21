#include "ComputePipeline.h"
#include "VulkanHelpers.h"
#include "VulkanDevice.h"
#include "Shader.h"
#include "DataHandling/Helper.h"

using namespace vkw;


ComputePipeline::ComputePipeline(VulkanDevice* pDevice, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, const std::string& computeShader)
	:m_pDevice(pDevice)
	, m_PipelineCache(pipelineCache)
	, m_DescriptorSetLayout(descriptorSetLayout)
	, m_ComputeShaderPath(computeShader)
{
	Init();
}

ComputePipeline::~ComputePipeline()
{
	Cleanup();
}

VkPipelineLayout ComputePipeline::GetLayout()
{
	return m_PipelineLayout;
}

VkPipeline ComputePipeline::GetPipeline()
{
	return m_Pipeline;
}

void ComputePipeline::Rebuild()
{
	Cleanup();
	Init();
}

void ComputePipeline::Init()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	ErrorCheck(vkCreatePipelineLayout(m_pDevice->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

	VkPipelineShaderStageCreateInfo shaderStage{};

	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStage.module = CreateShaderModule(readFile(m_ComputeShaderPath), m_pDevice->GetDevice());
	shaderStage.pName = "main";


	VkComputePipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = m_PipelineLayout;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.layout = m_PipelineLayout;

	
	ErrorCheck(vkCreateComputePipelines(m_pDevice->GetDevice(), m_PipelineCache, 1, &pipelineCreateInfo, nullptr, &m_Pipeline));

	vkDestroyShaderModule(m_pDevice->GetDevice(), shaderStage.module, nullptr);

}

void ComputePipeline::Cleanup()
{
	vkDestroyPipeline(m_pDevice->GetDevice(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_PipelineLayout, nullptr);
}
