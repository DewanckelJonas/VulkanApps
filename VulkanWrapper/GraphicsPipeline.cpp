#include "GraphicsPipeline.h"
#include "VulkanHelpers.h"
#include "VulkanDevice.h"
#include "RenderPass.h"
#include "VertexLayout.h"
#include "Shader.h"
#include "DataHandling/Helper.h"

using namespace vkw;


GraphicsPipeline::GraphicsPipeline(VulkanDevice* pDevice, RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, VertexLayout* pVertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace, bool wireframe)
	:m_pDevice(pDevice)
{
	Init(pRenderPass, pipelineCache, descriptorSetLayout, pVertexLayout, vertexShader, fragShader, topology, frontFace, wireframe);
}

GraphicsPipeline::~GraphicsPipeline()
{
	Cleanup();
}

VkPipelineLayout vkw::GraphicsPipeline::GetLayout()
{
	return m_PipelineLayout;
}

VkPipeline vkw::GraphicsPipeline::GetPipeline()
{
	return m_Pipeline;
}

void vkw::GraphicsPipeline::Init(RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, VertexLayout* pVertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace, bool wireframe)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	ErrorCheck(vkCreatePipelineLayout(m_pDevice->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = topology;


	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	if(wireframe)
	{
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
	}else
	{
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	}
	rasterizationState.frontFace = frontFace;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.f;



	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	std::array<VkPipelineColorBlendAttachmentState, 1> colorBlendAttachments{ colorBlendAttachment };

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlendState.attachmentCount = uint32_t(colorBlendAttachments.size());
	colorBlendState.pAttachments = colorBlendAttachments.data();
	colorBlendState.blendConstants[0] = 0.0f; // Optional
	colorBlendState.blendConstants[1] = 0.0f; // Optional
	colorBlendState.blendConstants[2] = 0.0f; // Optional
	colorBlendState.blendConstants[3] = 0.0f; // Optional

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.scissorCount = 1;

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	pipelineDynamicStateCreateInfo.dynamicStateCount = uint32_t(dynamicStateEnables.size());
	pipelineDynamicStateCreateInfo.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = CreateShaderModule(readFile(vertexShader), m_pDevice->GetDevice());
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = CreateShaderModule(readFile(fragShader), m_pDevice->GetDevice());
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = m_PipelineLayout;
	pipelineCreateInfo.renderPass = pRenderPass->GetHandle();
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.stageCount = uint32_t(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.layout = m_PipelineLayout;
	
	VkPipelineVertexInputStateCreateInfo emptyInputState{};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	if(pVertexLayout)
	{
		pipelineCreateInfo.pVertexInputState = &pVertexLayout->CreateVertexDescription();
	}

	ErrorCheck(vkCreateGraphicsPipelines(m_pDevice->GetDevice(), pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_Pipeline));

	vkDestroyShaderModule(m_pDevice->GetDevice(), shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_pDevice->GetDevice(), shaderStages[1].module, nullptr);

}

void vkw::GraphicsPipeline::Cleanup()
{
	vkDestroyPipeline(m_pDevice->GetDevice(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_PipelineLayout, nullptr);
}
