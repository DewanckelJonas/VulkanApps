#include "GraphicsPipeline.h"
#include "VulkanHelpers.h"
#include "VulkanDevice.h"
#include "RenderPass.h"
#include "VertexLayout.h"
#include "Shader.h"
#include "DataHandling/Helper.h"

using namespace vkw;


GraphicsPipeline::GraphicsPipeline(VulkanDevice* pDevice, RenderPass* pRenderPass, VkPipelineCache pipelineCache, VkDescriptorSetLayout descriptorSetLayout, const VertexLayout& vertexLayout, const std::string& vertexShader, const std::string& fragShader, VkPrimitiveTopology topology, VkFrontFace frontFace, bool wireframe)
	:m_pDevice(pDevice)
	,m_pRenderPass(pRenderPass)
	,m_PipelineCache(pipelineCache)
	,m_DescriptorSetLayout(descriptorSetLayout)
	,m_VertexLayout(vertexLayout)
	,m_VertexShaderPath(vertexShader)
	,m_FragmentShaderPath(fragShader)
	,m_Topology(topology)
	,m_FrontFace(frontFace)
	,m_Wireframe(wireframe)
{
	Init();
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

void vkw::GraphicsPipeline::Rebuild()
{
	Cleanup();
	Init();
}

void vkw::GraphicsPipeline::Init()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	ErrorCheck(vkCreatePipelineLayout(m_pDevice->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = m_Topology;


	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	if(m_Wireframe)
	{
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
	}else
	{
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	}
	rasterizationState.frontFace = m_FrontFace;
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

	//// Additive blending
	//colorBlendAttachments[0].colorWriteMask = 0xF;
	//colorBlendAttachments[0].blendEnable = VK_TRUE;
	//colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = uint32_t(colorBlendAttachments.size());
	colorBlendState.pAttachments = colorBlendAttachments.data();

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
	shaderStages[0].module = CreateShaderModule(readFile(m_VertexShaderPath), m_pDevice->GetDevice());
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = CreateShaderModule(readFile(m_FragmentShaderPath), m_pDevice->GetDevice());
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = m_PipelineLayout;
	pipelineCreateInfo.renderPass = m_pRenderPass->GetHandle();
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
	pipelineCreateInfo.pVertexInputState = &m_VertexLayout.CreateVertexDescription();

	ErrorCheck(vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_PipelineCache, 1, &pipelineCreateInfo, nullptr, &m_Pipeline));

	vkDestroyShaderModule(m_pDevice->GetDevice(), shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_pDevice->GetDevice(), shaderStages[1].module, nullptr);

}

void vkw::GraphicsPipeline::Cleanup()
{
	vkDestroyPipeline(m_pDevice->GetDevice(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_PipelineLayout, nullptr);
}
