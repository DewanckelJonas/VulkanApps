#include "VulkanApp.h"
#include "VulkanSwapchain.h"
#include "VulkanHelpers.h"
#include "Buffer.h"
#include "Texture.h"
#include "VulkanDevice.h"
#include "Shader.h"
#include "Helper.h"
#include <array>
#include "Window.h"
#include "RenderPass.h"
#include "CommandPool.h"
#include "FrameBuffer.h"
#include <sstream>
#include <algorithm>
#include "DescriptorSetLayout.h"
#include "DepthStencilBuffer.h"
#include "RaytracingGeometry.h"
#include <iostream>

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "Raytracing")
{
}


VulkanApp::~VulkanApp()
{
}

void VulkanApp::Render()
{
	GetSwapchain()->AcquireNextImage(GetPresentCompleteSemaphore());

	////ofscreen

	VkSubmitInfo submitInfo{};
	VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &waitStageFlag;
	submitInfo.pWaitSemaphores = &GetPresentCompleteSemaphore();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_Offscreen.semaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_Offscreen.commandBuffer;

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	//raytracing
	waitStageFlag = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &waitStageFlag;
	submitInfo.pWaitSemaphores = &m_Offscreen.semaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_Raytracing.semaphore;
	submitInfo.pCommandBuffers = &m_Raytracing.commandBuffer;

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	

	//onscreen
	waitStageFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &waitStageFlag;
	submitInfo.pWaitSemaphores = &m_Raytracing.semaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &GetRenderCompleteSemaphore();
	submitInfo.pCommandBuffers = &GetDrawCommandBuffers()[GetSwapchain()->GetActiveImageId()];

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));

	GetSwapchain()->PresentImage(GetRenderCompleteSemaphore());
}

bool VulkanApp::Update(float dTime)
{
	m_ElapsedSec += dTime;
	glm::vec2 mousePos = GetWindow()->GetMousePos();
	glm::vec2 mouseMovement = mousePos - m_PrevMousePos;
	m_PrevMousePos = mousePos;
	if(GetWindow()->IsKeyButtonDown(VK_LSHIFT))
	{
		m_Camera.SetMovementSpeed(10.f);
	}
	else
	{
		m_Camera.SetMovementSpeed(5.f);
	}
	Camera_Movement movementDir{Camera_Movement::NONE};
	if(GetWindow()->IsKeyButtonDown('W')) movementDir = Camera_Movement::FORWARD;
	if(GetWindow()->IsKeyButtonDown('S')) movementDir = Camera_Movement::BACKWARD;
	if(GetWindow()->IsKeyButtonDown('A')) movementDir = Camera_Movement::LEFT;
	if(GetWindow()->IsKeyButtonDown('D')) movementDir = Camera_Movement::RIGHT;
	m_Camera.ProcessKeyboard(movementDir, dTime);
	float sensitivity = 3.f;
	if(GetWindow()->IsKeyButtonDown(VK_LBUTTON))
	{
		m_Camera.ProcessMouseMovement(sensitivity * mouseMovement.x, sensitivity * mouseMovement.y, true);
	}
	UpdateUniformBuffers();
	return VulkanBaseApp::Update(dTime);
}


void VulkanApp::Init(uint32_t width, uint32_t height)
{
	EnableRaytracingExtension();
	VulkanBaseApp::Init(width, height);
	GetRaytracingFunctionPointers();
	CreateRaytracingPipeline();
	CreateRaytracingAttachment();
	CreateShaderBindingTable();
	CreateDisplayQuads();
	CreateOffscreenFrameBuffers();
	CreateUniformBuffers();
	CreateRaytracingScene();
	CreateDescriptorSetLayout();
	CreatePipelines();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateRaytracingDescriptorSet();
	BuildOffscreenCommandBuffers();
	BuildDrawCommandBuffers();
	CreateRaytracingCommandBuffer();
	m_PrevMousePos = GetWindow()->GetMousePos();
}

void VulkanApp::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	DestroyOffscreenCommandBuffer();
	DestroyDescriptorPool();
	DestroyRaytracingScene();
	DestroyRaytracingAttachment();
	DestroyRaytracinDescriptorSet();
	DestroyRaytracingPipeline();
	DestroyPipelines();
	DestroyDescriptorSetLayout();
	DestroyUniformBuffers();
	DestroyOffscreenFrameBuffers();
	DestroyDisplayQuads();
	VulkanBaseApp::Cleanup();
}


void VulkanApp::CreateUniformBuffers()
{
	//Update Deferred Buffer
	m_DeferredUniformBufferData.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	m_DeferredUniformBufferData.model = glm::mat4(1.0f);

	m_OffscreenUniformBufferData.projection = m_Camera.GetProjectionMatrix(GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height, 0.1f, 250);
	m_OffscreenUniformBufferData.instancePos[0] = glm::vec4(0.0f);
	m_OffscreenUniformBufferData.instancePos[1] = glm::vec4(-4.0f, 0.0, -4.0f, 0.0f);
	m_OffscreenUniformBufferData.instancePos[2] = glm::vec4(4.0f, 0.0, -4.0f, 0.0f);

	// Current view position
	m_DeferredLights.viewPos = glm::vec4(0.f, 0.f, 0.f, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

	m_pDeferredBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(m_DeferredUniformBufferData), &m_DeferredUniformBufferData);

	m_pOffscreenBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(m_OffscreenUniformBufferData), &m_OffscreenUniformBufferData);

	m_pLightBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(m_DeferredLights), &m_DeferredLights);
	

	// Update
	UpdateUniformBuffers();
}

void VulkanApp::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 8;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 9;

	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = poolSizes.size();
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = 3;

	ErrorCheck(vkCreateDescriptorPool(GetDevice()->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));
}

void VulkanApp::CreateDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.pSetLayouts = &m_DescriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &allocInfo, &m_DescriptorSet));

	// Image descriptors for the offscreen color attachments
	VkDescriptorImageInfo texDescriptorPosition{};
	texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorPosition.imageView = m_Offscreen.posAttachment->GetImageView();
	texDescriptorPosition.sampler = m_Offscreen.posAttachment->GetDescriptor().sampler;
		

	VkDescriptorImageInfo texDescriptorNormal{};
	texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorNormal.imageView = m_Offscreen.normAttachment->GetImageView();
	texDescriptorNormal.sampler = m_Offscreen.normAttachment->GetDescriptor().sampler;

	VkDescriptorImageInfo texDescriptorAlbedo{};
	texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorAlbedo.imageView = m_Offscreen.albedoAttachment->GetImageView();
	texDescriptorAlbedo.sampler = m_Offscreen.albedoAttachment->GetDescriptor().sampler;


	std::vector<VkWriteDescriptorSet> deferredWriteDescriptorSets{6};
	//uniform buffer
	deferredWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	deferredWriteDescriptorSets[0].dstBinding = 0;
	deferredWriteDescriptorSets[0].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[0].pBufferInfo = &m_pDeferredBuffer->GetDescriptor(); 
	deferredWriteDescriptorSets[0].descriptorCount = 1;

	//pos
	deferredWriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredWriteDescriptorSets[1].dstBinding = 1;
	deferredWriteDescriptorSets[1].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[1].pImageInfo = &texDescriptorPosition;
	deferredWriteDescriptorSets[1].descriptorCount = 1;

	//norms
	deferredWriteDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredWriteDescriptorSets[2].dstBinding = 2;
	deferredWriteDescriptorSets[2].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[2].pImageInfo = &texDescriptorNormal;
	deferredWriteDescriptorSets[2].descriptorCount = 1;

	//albedos
	deferredWriteDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredWriteDescriptorSets[3].dstBinding = 3;
	deferredWriteDescriptorSets[3].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[3].pImageInfo = &texDescriptorAlbedo;
	deferredWriteDescriptorSets[3].descriptorCount = 1;

	//lights
	deferredWriteDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	deferredWriteDescriptorSets[4].dstBinding = 4;
	deferredWriteDescriptorSets[4].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[4].pBufferInfo = &m_pLightBuffer->GetDescriptor();
	deferredWriteDescriptorSets[4].descriptorCount = 1;
	
	//shadows
	deferredWriteDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	deferredWriteDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredWriteDescriptorSets[5].dstBinding = 5;
	deferredWriteDescriptorSets[5].dstSet = m_DescriptorSet;
	deferredWriteDescriptorSets[5].pImageInfo = &m_Raytracing.attachment->GetDescriptor();
	deferredWriteDescriptorSets[5].descriptorCount = 1;


	vkUpdateDescriptorSets(GetDevice()->GetDevice(), static_cast<uint32_t>(deferredWriteDescriptorSets.size()), deferredWriteDescriptorSets.data(), 0, nullptr);

	// Offscreen (scene)

	// Model
	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &allocInfo, &m_Offscreen.modelDescriptorSet));
	std::vector<VkWriteDescriptorSet> offscreenWriteDescriptorSets{ 3 };
	offscreenWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[0].dstSet = m_Offscreen.modelDescriptorSet;
	offscreenWriteDescriptorSets[0].dstBinding = 0;
	offscreenWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	offscreenWriteDescriptorSets[0].pBufferInfo = &m_pOffscreenBuffer->GetDescriptor();
	offscreenWriteDescriptorSets[0].descriptorCount = 1;

	offscreenWriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[1].dstSet = m_Offscreen.modelDescriptorSet;
	offscreenWriteDescriptorSets[1].dstBinding = 1;
	offscreenWriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	offscreenWriteDescriptorSets[1].pImageInfo = &m_pArmorAlbedo->GetDescriptor();
	offscreenWriteDescriptorSets[1].descriptorCount = 1;

	offscreenWriteDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[2].dstSet = m_Offscreen.modelDescriptorSet;
	offscreenWriteDescriptorSets[2].dstBinding = 2;
	offscreenWriteDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	offscreenWriteDescriptorSets[2].pImageInfo = &m_pArmorNormal->GetDescriptor();
	offscreenWriteDescriptorSets[2].descriptorCount = 1;

	vkUpdateDescriptorSets(GetDevice()->GetDevice(), static_cast<uint32_t>(offscreenWriteDescriptorSets.size()), offscreenWriteDescriptorSets.data(), 0, NULL);

	// Background
	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &allocInfo, &m_Offscreen.backgroundDescriptorSet));
	offscreenWriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[0].dstSet = m_Offscreen.backgroundDescriptorSet;
	offscreenWriteDescriptorSets[0].dstBinding = 0;
	offscreenWriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	offscreenWriteDescriptorSets[0].pBufferInfo = &m_pOffscreenBuffer->GetDescriptor();
	offscreenWriteDescriptorSets[0].descriptorCount = 1;

	offscreenWriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[1].dstSet = m_Offscreen.backgroundDescriptorSet;
	offscreenWriteDescriptorSets[1].dstBinding = 1;
	offscreenWriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	offscreenWriteDescriptorSets[1].pImageInfo = &m_pFloorAlbedo->GetDescriptor();
	offscreenWriteDescriptorSets[1].descriptorCount = 1;

	offscreenWriteDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	offscreenWriteDescriptorSets[2].dstSet = m_Offscreen.backgroundDescriptorSet;
	offscreenWriteDescriptorSets[2].dstBinding = 2;
	offscreenWriteDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	offscreenWriteDescriptorSets[2].pImageInfo = &m_pFloorNormal->GetDescriptor();
	offscreenWriteDescriptorSets[2].descriptorCount = 1;

	vkUpdateDescriptorSets(GetDevice()->GetDevice(), static_cast<uint32_t>(offscreenWriteDescriptorSets.size()), offscreenWriteDescriptorSets.data(), 0, NULL);
}

void VulkanApp::CreateDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{ 6 };
	//uniform buffer
	setLayoutBindings[0] = {};
	setLayoutBindings[0].binding = 0;
	setLayoutBindings[0].descriptorCount = 1;
	setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//pos
	setLayoutBindings[1] = {};
	setLayoutBindings[1].binding = 1;
	setLayoutBindings[1].descriptorCount = 1;
	setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//normals
	setLayoutBindings[2] = {};
	setLayoutBindings[2].binding = 2;
	setLayoutBindings[2].descriptorCount = 1;
	setLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//albedos
	setLayoutBindings[3] = {};
	setLayoutBindings[3].binding = 3;
	setLayoutBindings[3].descriptorCount = 1;
	setLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//lights
	setLayoutBindings[4] = {};
	setLayoutBindings[4].binding = 4;
	setLayoutBindings[4].descriptorCount = 1;
	setLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//shadows
	setLayoutBindings[5] = {};
	setLayoutBindings[5].binding = 5;
	setLayoutBindings[5].descriptorCount = 1;
	setLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = setLayoutBindings.size();
	setLayoutCreateInfo.pBindings = setLayoutBindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(GetDevice()->GetDevice(), &setLayoutCreateInfo, nullptr, &m_DescriptorSetLayout));
	vkQueueWaitIdle(GetDevice()->GetQueue());
}

void VulkanApp::EnableRaytracingExtension()
{
	GetDevice()->EnableInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);
}

void VulkanApp::GetRaytracingFunctionPointers()
{
	// Get VK_NV_ray_tracing related function pointers
	vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(GetDevice()->GetDevice(), "vkCreateRayTracingPipelinesNV"));
	vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(GetDevice()->GetDevice(), "vkGetRayTracingShaderGroupHandlesNV"));
	vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(GetDevice()->GetDevice(), "vkCmdTraceRaysNV"));
}

void VulkanApp::CreateOffscreenRenderPass()
{
	std::vector<VkAttachmentDescription>  attachmentDescs{4};

	// Init attachment properties
	for (uint32_t i = 0; i < 3; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	attachmentDescs[3].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescs[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescs[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescs[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescs[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescs[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescs[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Formats
	attachmentDescs[0].format = m_Offscreen.posAttachment->GetFormat();
	attachmentDescs[1].format = m_Offscreen.normAttachment->GetFormat();
	attachmentDescs[2].format = m_Offscreen.albedoAttachment->GetFormat();
	attachmentDescs[3].format = m_Offscreen.depthAttachment->GetFormat();

	std::vector<VkAttachmentReference> colorReferences;
	colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 3;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	subpass.pDepthStencilAttachment = &depthReference;

	std::vector<VkSubpassDependency> dependencies{2};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	m_Offscreen.renderPass = new vkw::RenderPass(GetDevice(), GetWindow(), attachmentDescs, std::vector<VkSubpassDescription>{ subpass }, dependencies);
}

void VulkanApp::CreateOffscreenFrameBuffers()
{
	vkw::Texture::TextureProperties properties{};
	properties.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	properties.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	properties.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	properties.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	properties.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

	m_Offscreen.posAttachment = new vkw::Texture(GetDevice(), GetCommandPool(), properties,
		nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height);

	m_Offscreen.normAttachment = new vkw::Texture(GetDevice(), GetCommandPool(), properties,
		nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height);

	//albedo attachment does not nee the 16 bit precision
	properties.format = VK_FORMAT_R8G8B8A8_UNORM;

	m_Offscreen.albedoAttachment = new vkw::Texture(GetDevice(), GetCommandPool(), properties,
		nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height);

	m_Offscreen.depthAttachment = new vkw::DepthStencilBuffer(GetDevice(), GetWindow());

	std::vector<VkImageView> frameBufferAttachments{4};
	frameBufferAttachments[0] = m_Offscreen.posAttachment->GetImageView();
	frameBufferAttachments[1] = m_Offscreen.normAttachment->GetImageView();
	frameBufferAttachments[2] = m_Offscreen.albedoAttachment->GetImageView();
	frameBufferAttachments[3] = m_Offscreen.depthAttachment->GetImageView();

	CreateOffscreenRenderPass();

	m_Offscreen.frameBuffer = new vkw::FrameBuffer(GetDevice(), m_Offscreen.renderPass, GetWindow(), frameBufferAttachments);
}

void VulkanApp::CreatePipelines()
{
	//deferred
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	ErrorCheck(vkCreatePipelineLayout(GetDevice()->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_DeferredPipelineLayout));

	// Offscreen layout
	VkPipelineLayoutCreateInfo offscreenPipelineLayout{};
	offscreenPipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	offscreenPipelineLayout.pSetLayouts = &m_DescriptorSetLayout;
	offscreenPipelineLayout.setLayoutCount = 1;
	ErrorCheck(vkCreatePipelineLayout(GetDevice()->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_Offscreen.pipelineLayout));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;


	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.f;

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
	pipelineColorBlendAttachmentState.colorWriteMask = 0xf;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.attachmentCount = 1;
	pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;

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
	pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();
	pipelineDynamicStateCreateInfo.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};


	// Final fullscreen composition pass pipeline
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = CreateShaderModule(readFile("shaders/deferred.vert.spv"), GetDevice()->GetDevice());
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = CreateShaderModule(readFile("shaders/deferred.frag.spv"), GetDevice()->GetDevice());
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = m_DeferredPipelineLayout;
	pipelineCreateInfo.renderPass = GetRenderPass()->GetHandle();
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();


	VkPipelineVertexInputStateCreateInfo emptyInputState{};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	pipelineCreateInfo.layout = m_DeferredPipelineLayout;

	ErrorCheck(vkCreateGraphicsPipelines(GetDevice()->GetDevice(), GetPipelineCache(), 1, &pipelineCreateInfo, nullptr, &m_DeferredPipeline));

	vkDestroyShaderModule(GetDevice()->GetDevice(), shaderStages[0].module, nullptr);
	vkDestroyShaderModule(GetDevice()->GetDevice(), shaderStages[1].module, nullptr);

	// Offscreen pipeline
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = CreateShaderModule(readFile("shaders/mrt.vert.spv"), GetDevice()->GetDevice());
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = CreateShaderModule(readFile("shaders/mrt.frag.spv"), GetDevice()->GetDevice());
	shaderStages[1].pName = "main";

	// Offscreen render pass
	pipelineCreateInfo.renderPass = m_Offscreen.renderPass->GetHandle();
	pipelineCreateInfo.pVertexInputState = &m_VertexLayout.CreateVertexDescription();
	pipelineCreateInfo.layout = m_Offscreen.pipelineLayout;

	//we have to set the color blend write mask otherwise its 0x0 and we cant render
	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates{};

	blendAttachmentStates[0].colorWriteMask = 0xf;
	blendAttachmentStates[0].blendEnable = VK_FALSE;

	blendAttachmentStates[1].colorWriteMask = 0xf;
	blendAttachmentStates[1].blendEnable = VK_FALSE;

	blendAttachmentStates[2].colorWriteMask = 0xf;
	blendAttachmentStates[2].blendEnable = VK_FALSE;

	pipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	pipelineColorBlendStateCreateInfo.pAttachments = blendAttachmentStates.data();

	ErrorCheck(vkCreateGraphicsPipelines(GetDevice()->GetDevice(), GetPipelineCache(), 1, &pipelineCreateInfo, nullptr, &m_Offscreen.pipeline));

	vkDestroyShaderModule(GetDevice()->GetDevice(), shaderStages[0].module, nullptr);
	vkDestroyShaderModule(GetDevice()->GetDevice(), shaderStages[1].module, nullptr);
}


void VulkanApp::DestroyPipelines()
{
	vkDestroyPipeline(GetDevice()->GetDevice(), m_Offscreen.pipeline, nullptr);
	vkDestroyPipeline(GetDevice()->GetDevice(), m_DeferredPipeline, nullptr);

	vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_DeferredPipelineLayout, nullptr);
	vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_Offscreen.pipelineLayout, nullptr);

	delete m_Offscreen.albedoAttachment;
	delete m_Offscreen.normAttachment;
	delete m_Offscreen.posAttachment;
	delete m_Offscreen.depthAttachment;
	
	vkDestroySemaphore(GetDevice()->GetDevice(), m_Offscreen.semaphore, nullptr); 
}

void VulkanApp::DestroyDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
}

void VulkanApp::DestroyDisplayQuads()
{
	delete m_pDisplayVertexBuffer;
	delete m_pDisplayIndexBuffer;
}

void VulkanApp::DestroyOffscreenFrameBuffers()
{
	delete m_Offscreen.frameBuffer;
	delete m_Offscreen.renderPass;
}

void VulkanApp::DestroyUniformBuffers()
{
	delete m_pOffscreenBuffer;
	delete m_pLightBuffer;
	delete m_pDeferredBuffer;
}


void VulkanApp::DestroyDescriptorPool()
{
	vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_DescriptorPool, nullptr);
}

void VulkanApp::DestroyOffscreenCommandBuffer()
{
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), 1, &m_Offscreen.commandBuffer);
}

void VulkanApp::DestroyRaytracingScene()
{
	delete m_Raytracing.scene;
}

void VulkanApp::DestroyRaytracingAttachment()
{
	delete m_Raytracing.attachment;
}

void VulkanApp::DestroyRaytracinDescriptorSet()
{
	vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_Raytracing.descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_Raytracing.descriptorSetLayout, nullptr);
}

void VulkanApp::DestroyRaytracingPipeline()
{
	vkDestroySemaphore(GetDevice()->GetDevice(), m_Raytracing.semaphore, nullptr);
	delete m_Raytracing.shaderBindingTable;
	vkDestroyPipeline(GetDevice()->GetDevice(), m_Raytracing.pipeline, nullptr);
	vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_Raytracing.pipelineLayout, nullptr);
}




void VulkanApp::BuildDrawCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;;
	renderPassBeginInfo.renderPass = GetRenderPass()->GetHandle();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = GetWindow()->GetSurfaceSize();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	
	for (int32_t i = 0; i < GetDrawCommandBuffers().size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(GetDrawCommandBuffers()[i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(GetDrawCommandBuffers()[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.width = GetWindow()->GetSurfaceSize().width;
		viewport.height = GetWindow()->GetSurfaceSize().height;
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(GetDrawCommandBuffers()[i], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = GetWindow()->GetSurfaceSize();
		scissor.offset = { 0, 0 };

		vkCmdSetScissor(GetDrawCommandBuffers()[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindDescriptorSets(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_DeferredPipelineLayout, 0, 1, &m_DescriptorSet, 0, NULL);


		// Final composition as full screen quad
		vkCmdBindPipeline(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_DeferredPipeline);
		vkCmdBindIndexBuffer(GetDrawCommandBuffers()[i], m_pDisplayIndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32); //insert quad index buffer;
		vkCmdDrawIndexed(GetDrawCommandBuffers()[i], 6, 1, 0, 0, 1);


		vkCmdEndRenderPass(GetDrawCommandBuffers()[i]);

		ErrorCheck(vkEndCommandBuffer(GetDrawCommandBuffers()[i]));
	}
}

void VulkanApp::BuildOffscreenCommandBuffers()
{
	if (m_Offscreen.commandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = GetCommandPool()->GetHandle();
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &cmdBufferAllocInfo, &m_Offscreen.commandBuffer));
		// Create a semaphore used to synchronize offscreen rendering and usage
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		ErrorCheck(vkCreateSemaphore(GetDevice()->GetDevice(), &semaphoreCreateInfo, nullptr, &m_Offscreen.semaphore));
	}


	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Clear values for all attachments written in the fragment sahder
	std::array<VkClearValue, 4> clearValues;
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_Offscreen.renderPass->GetHandle();
	renderPassBeginInfo.framebuffer = m_Offscreen.frameBuffer->GetHandle();
	renderPassBeginInfo.renderArea.extent = GetWindow()->GetSurfaceSize();
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	ErrorCheck(vkBeginCommandBuffer(m_Offscreen.commandBuffer, &cmdBufferBeginInfo));

	vkCmdBeginRenderPass(m_Offscreen.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.width = GetWindow()->GetSurfaceSize().width;
	viewport.height = GetWindow()->GetSurfaceSize().height;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(m_Offscreen.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent = GetWindow()->GetSurfaceSize();
	scissor.offset = { 0, 0 };

	vkCmdSetScissor(m_Offscreen.commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(m_Offscreen.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Offscreen.pipeline);

	VkDeviceSize offsets[1] = { 0 };

	// Background
	vkCmdBindDescriptorSets(m_Offscreen.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Offscreen.pipelineLayout, 0, 1, &m_Offscreen.backgroundDescriptorSet, 0, NULL);
	vkCmdBindVertexBuffers(m_Offscreen.commandBuffer, 0, 1, &m_pFloorModel->GetVertexBuffer()->GetHandle(), offsets);
	vkCmdBindIndexBuffer(m_Offscreen.commandBuffer, m_pFloorModel->GetIndexBuffer()->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_Offscreen.commandBuffer, m_pFloorModel->GetIndexCount(), 1, 0, 0, 0);

	// Object
	vkCmdBindDescriptorSets(m_Offscreen.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Offscreen.pipelineLayout, 0, 1, &m_Offscreen.modelDescriptorSet, 0, NULL);
	vkCmdBindVertexBuffers(m_Offscreen.commandBuffer, 0, 1, &m_pArmorModel->GetVertexBuffer()->GetHandle(), offsets); 
	vkCmdBindIndexBuffer(m_Offscreen.commandBuffer, m_pArmorModel->GetIndexBuffer()->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_Offscreen.commandBuffer, m_pArmorModel->GetIndexCount(), 3, 0, 0, 0);

	vkCmdEndRenderPass(m_Offscreen.commandBuffer);

	ErrorCheck(vkEndCommandBuffer(m_Offscreen.commandBuffer));
}

void VulkanApp::CreateDisplayQuads()
{
	// Setup vertices for multiple screen aligned quads
		// Used for displaying final result and debug 
	struct Vertex {
		float pos[3];
		float uv[2];
		float col[3];
		float normal[3];
		float tangent[3];
	};

	std::vector<Vertex> vertexBuffer;

	float x = 0.0f;
	float y = 0.0f;
	for (uint32_t i = 0; i < 3; i++)
	{
		// Last component of normal is used for debug display sampler index
		vertexBuffer.push_back({ { x + 1.0f, y + 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
		vertexBuffer.push_back({ { x,      y + 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
		vertexBuffer.push_back({ { x,      y,      0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
		vertexBuffer.push_back({ { x + 1.0f, y,      0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
		x += 1.0f;
		if (x > 1.0f)
		{
			x = 0.0f;
			y += 1.0f;
		}
	}

	m_pDisplayVertexBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer.size(), vertexBuffer.data());


	// Setup indices
	std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
	for (uint32_t i = 0; i < 3; ++i)
	{
		uint32_t indices[6] = { 0,1,2, 2,3,0 };
		for (auto index : indices)
		{
			indexBuffer.push_back(i * 4 + index);
		}
	}
	m_DisplayIndexCount = static_cast<uint32_t>(indexBuffer.size());

	m_pDisplayIndexBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer.size(), indexBuffer.data());
}


void VulkanApp::CreateRaytracingScene()
{
	std::vector<vkw::GeometryInstance> geometryInstances{4};

	for (size_t i = 0; i < 3; i++)
	{
		glm::mat3x4 transform = {
			1.0f, 0.0f, 0.0f, m_OffscreenUniformBufferData.instancePos[i].x,
			0.0f, -1.0f, 0.0f, m_OffscreenUniformBufferData.instancePos[i].y,
			0.0f, 0.0f, 1.0f, m_OffscreenUniformBufferData.instancePos[i].z,
		};

		geometryInstances[i].transform = transform;
		geometryInstances[i].instanceId = 0;
		geometryInstances[i].mask = 0xff;
		geometryInstances[i].instanceOffset = 0;
		geometryInstances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		//insert BLAS id in the vector of BLAS that you want an instance of here. Will internally be replaced with handle
		geometryInstances[i].bottomLevelHandle = 0;

	}

	glm::mat3x4 transform = {
			1.0f, 0.0f, 0.0f, 0.f,
			0.0f, -1.0f, 0.0f, 0.f,
			0.0f, 0.0f, 1.0f, 0.f,
	};

	geometryInstances[3].transform = transform;
	geometryInstances[3].instanceId = 0;
	geometryInstances[3].mask = 0xff;
	geometryInstances[3].instanceOffset = 0;
	geometryInstances[3].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	geometryInstances[3].bottomLevelHandle = 1;

	std::vector<vkw::Model*> scene{};
	scene.push_back(m_pArmorModel);
	scene.push_back(m_pFloorModel);
	m_Raytracing.scene = new vkw::RaytracingGeometry(GetDevice(), GetCommandPool(), scene, geometryInstances);

}

void VulkanApp::CreateRaytracingDescriptorSet()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
	descriptorPoolCreateInfo.maxSets = 1;

	ErrorCheck(vkCreateDescriptorPool(GetDevice()->GetDevice(), &descriptorPoolCreateInfo, nullptr, &m_Raytracing.descriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_Raytracing.descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &m_Raytracing.descriptorSetLayout;

	ErrorCheck(vkAllocateDescriptorSets(GetDevice()->GetDevice(), &descriptorSetAllocateInfo, &m_Raytracing.descriptorSet));

	VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &m_Raytracing.scene->GetTopLevelAS()->GetStructure();

	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = m_Raytracing.descriptorSet;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

	VkWriteDescriptorSet renderImageWrite{};
	renderImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	renderImageWrite.dstSet = m_Raytracing.descriptorSet;
	renderImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	renderImageWrite.dstBinding = 1;
	renderImageWrite.descriptorCount = 1;
	renderImageWrite.pImageInfo = &m_Raytracing.attachment->GetDescriptor();

	VkWriteDescriptorSet uniformBufferWrite{};
	uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformBufferWrite.dstSet = m_Raytracing.descriptorSet;
	uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferWrite.descriptorCount = 1;
	uniformBufferWrite.dstBinding = 2;
	uniformBufferWrite.pBufferInfo = &m_pLightBuffer->GetDescriptor();

	VkDescriptorImageInfo texDescriptorPosition{};
	texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorPosition.imageView = m_Offscreen.posAttachment->GetImageView();
	texDescriptorPosition.sampler = m_Offscreen.posAttachment->GetDescriptor().sampler;

	VkWriteDescriptorSet posAttachmentWrite{};
	posAttachmentWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	posAttachmentWrite.dstSet = m_Raytracing.descriptorSet;
	posAttachmentWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	posAttachmentWrite.descriptorCount = 1;
	posAttachmentWrite.dstBinding = 3;
	posAttachmentWrite.pImageInfo = &texDescriptorPosition;

	VkDescriptorImageInfo texDescriptorNormal{};
	texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorNormal.imageView = m_Offscreen.normAttachment->GetImageView();
	texDescriptorNormal.sampler = m_Offscreen.normAttachment->GetDescriptor().sampler;

	VkWriteDescriptorSet normalTextureWrite{};
	normalTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	normalTextureWrite.dstSet = m_Raytracing.descriptorSet;
	normalTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalTextureWrite.descriptorCount = 1;
	normalTextureWrite.dstBinding = 4;
	normalTextureWrite.pImageInfo = &texDescriptorNormal;

	VkDescriptorImageInfo texDescriptorAlbedo{};
	texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texDescriptorAlbedo.imageView = m_Offscreen.albedoAttachment->GetImageView();
	texDescriptorAlbedo.sampler = m_Offscreen.albedoAttachment->GetDescriptor().sampler;


	VkWriteDescriptorSet albedoTextureWrite{};
	albedoTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	albedoTextureWrite.dstSet = m_Raytracing.descriptorSet;
	albedoTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedoTextureWrite.descriptorCount = 1;
	albedoTextureWrite.dstBinding = 5;
	albedoTextureWrite.pImageInfo = &texDescriptorAlbedo;

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		accelerationStructureWrite,
		renderImageWrite,
		uniformBufferWrite,
		posAttachmentWrite,
		normalTextureWrite,
		albedoTextureWrite
	};

	vkUpdateDescriptorSets(GetDevice()->GetDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void VulkanApp::CreateRaytracingPipeline()
{

	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
	accelerationStructureLayoutBinding.binding = 0;
	accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	accelerationStructureLayoutBinding.descriptorCount = 1;
	accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
	resultImageLayoutBinding.binding = 1;
	resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageLayoutBinding.descriptorCount = 1;
	resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding uniformBufferBinding{};
	uniformBufferBinding.binding = 2;
	uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferBinding.descriptorCount = 1;
	uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding posImageBinding{};
	posImageBinding.binding = 3;
	posImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	posImageBinding.descriptorCount = 1;
	posImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding normalImageBinding{};
	normalImageBinding.binding = 4;
	normalImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalImageBinding.descriptorCount = 1;
	normalImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding albedoImageBinding{};
	albedoImageBinding.binding = 5;
	albedoImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedoImageBinding.descriptorCount = 1;
	albedoImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	std::vector<VkDescriptorSetLayoutBinding> bindings({
		accelerationStructureLayoutBinding,
		resultImageLayoutBinding,
		uniformBufferBinding,
		posImageBinding,
		normalImageBinding,
		albedoImageBinding
		});

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	ErrorCheck(vkCreateDescriptorSetLayout(GetDevice()->GetDevice(), &layoutInfo, nullptr, &m_Raytracing.descriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_Raytracing.descriptorSetLayout;

	ErrorCheck(vkCreatePipelineLayout(GetDevice()->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_Raytracing.pipelineLayout));

	const uint32_t shaderIndexRaygen = 0;
	const uint32_t shaderIndexShadowMiss = 1;
	const uint32_t shaderIndexShadowHit = 2;
	

	std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages{};

	shaderStages[shaderIndexRaygen].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[shaderIndexRaygen].stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
	shaderStages[shaderIndexRaygen].module = CreateShaderModule(readFile("shaders/raygen.rgen.spv"), GetDevice()->GetDevice());
	shaderStages[shaderIndexRaygen].pName = "main";

	shaderStages[shaderIndexShadowMiss].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[shaderIndexShadowMiss].stage = VK_SHADER_STAGE_MISS_BIT_NV;
	shaderStages[shaderIndexShadowMiss].module = CreateShaderModule(readFile("shaders/miss.rmiss.spv"), GetDevice()->GetDevice());
	shaderStages[shaderIndexShadowMiss].pName = "main";

	shaderStages[shaderIndexShadowHit].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[shaderIndexShadowHit].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
	shaderStages[shaderIndexShadowHit].module = CreateShaderModule(readFile("shaders/closesthit.rchit.spv"), GetDevice()->GetDevice());
	shaderStages[shaderIndexShadowHit].pName = "main";

	/*
		Setup ray tracing shader groups
	*/
	std::vector<VkRayTracingShaderGroupCreateInfoNV> groups{ m_Raytracing.nrOfGroups };
	for (auto& group : groups) {
		// Init all groups with some default values
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		group.generalShader = VK_SHADER_UNUSED_NV;
		group.closestHitShader = VK_SHADER_UNUSED_NV;
		group.anyHitShader = VK_SHADER_UNUSED_NV;
		group.intersectionShader = VK_SHADER_UNUSED_NV;
	}
	const int rayGenGroupId = 0;
	const int shadowMissGroupId = 1;
	const int shadowClosestHitGroupId = 2;

	// Links shaders and types to ray tracing shader groups
	// Ray generation shader group
	groups[rayGenGroupId].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groups[rayGenGroupId].generalShader = shaderIndexRaygen;
	// Shadow closest hit shader group
	groups[shadowMissGroupId].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[shadowMissGroupId].generalShader = shaderIndexShadowMiss;
	groups[shadowClosestHitGroupId].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groups[shadowClosestHitGroupId].generalShader = VK_SHADER_UNUSED_NV;
	groups[shadowClosestHitGroupId].closestHitShader = shaderIndexShadowHit;

	VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size()); 
	rayPipelineInfo.pStages = shaderStages.data();
	rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
	rayPipelineInfo.pGroups = groups.data();
	rayPipelineInfo.maxRecursionDepth = 1;
	rayPipelineInfo.layout = m_Raytracing.pipelineLayout;

	ErrorCheck(vkCreateRayTracingPipelinesNV(GetDevice()->GetDevice(), VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_Raytracing.pipeline));

	for (size_t i = 0; i < shaderStages.size(); i++)
	{
		vkDestroyShaderModule(GetDevice()->GetDevice(), shaderStages[i].module, nullptr);
	}
}

void VulkanApp::CreateShaderBindingTable()
{
	m_Raytracing.deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	VkPhysicalDeviceProperties2 deviceProps2{};
	deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps2.pNext = &m_Raytracing.deviceProperties;
	vkGetPhysicalDeviceProperties2(GetDevice()->GetPhysicalDevice(), &deviceProps2);

	const uint32_t sbtSize = m_Raytracing.deviceProperties.shaderGroupHandleSize * m_Raytracing.nrOfGroups;
	uint8_t* shaderHandleStorage = new uint8_t[sbtSize];

	// Get shader identifiers
	ErrorCheck(vkGetRayTracingShaderGroupHandlesNV(GetDevice()->GetDevice(), m_Raytracing.pipeline, 0, m_Raytracing.nrOfGroups, sbtSize, shaderHandleStorage));

	// Create buffer for the shader binding table
	m_Raytracing.shaderBindingTable = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sbtSize, shaderHandleStorage);

}

void VulkanApp::CreateRaytracingCommandBuffer()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(GetDevice()->GetDevice(), &semaphoreCreateInfo, nullptr, &m_Raytracing.semaphore);

	VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = GetCommandPool()->GetHandle();
	cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &cmdBufferAllocInfo, &m_Raytracing.commandBuffer));

	VkCommandBufferBeginInfo cmdBufInfo{};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	ErrorCheck(vkBeginCommandBuffer(m_Raytracing.commandBuffer, &cmdBufInfo));

	vkCmdBindPipeline(m_Raytracing.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_Raytracing.pipeline);
	vkCmdBindDescriptorSets(m_Raytracing.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_Raytracing.pipelineLayout, 0, 1, &m_Raytracing.descriptorSet, 0, 0);

	VkDeviceSize bindingOffsetRayGenShader = m_Raytracing.deviceProperties.shaderGroupHandleSize * 0;
	VkDeviceSize bindingOffsetMissShader = m_Raytracing.deviceProperties.shaderGroupHandleSize * 1;
	VkDeviceSize bindingClosestHitShader = m_Raytracing.deviceProperties.shaderGroupHandleSize * 2;
	VkDeviceSize bindingStride = m_Raytracing.deviceProperties.shaderGroupHandleSize;

	vkCmdTraceRaysNV(m_Raytracing.commandBuffer,
	m_Raytracing.shaderBindingTable->GetHandle(), bindingOffsetRayGenShader,
	m_Raytracing.shaderBindingTable->GetHandle(), bindingOffsetMissShader, bindingStride,
	m_Raytracing.shaderBindingTable->GetHandle(), bindingClosestHitShader, bindingStride,
	VK_NULL_HANDLE, 0, 0,
	GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height, 1);

	ErrorCheck(vkEndCommandBuffer(m_Raytracing.commandBuffer));
}

void VulkanApp::CreateRaytracingAttachment()
{
	vkw::Texture::TextureProperties properties{};
	properties.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	properties.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	properties.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	properties.format = VK_FORMAT_R8G8B8A8_UNORM;
	properties.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	m_Raytracing.attachment = new vkw::Texture(GetDevice(), GetCommandPool(), properties, nullptr, GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height, m_NumberOfPrevFrames);
}

void VulkanApp::UpdateUniformBuffers()
{
	//Update Offscreen buffer
	m_OffscreenUniformBufferData.view = m_Camera.GetViewMatrix();
	m_OffscreenUniformBufferData.model = glm::mat4(1.0f);
	m_OffscreenUniformBufferData.projection = m_Camera.GetProjectionMatrix(GetWindow()->GetSurfaceSize().width, GetWindow()->GetSurfaceSize().height, 0.1f, 250);

	m_pOffscreenBuffer->Update(&m_OffscreenUniformBufferData, sizeof(m_OffscreenUniformBufferData), GetCommandPool());
	
	m_DeferredLights.lights[0].position = glm::vec4(0.0f, 10.0f, 10.0f, 0.0f);
	m_DeferredLights.lights[0].color = glm::vec3(1.f, 1.f, 1.f);
	m_DeferredLights.lights[0].radiance = 40.0f;
	m_DeferredLights.lights[0].radius = 1.f;
	m_DeferredLights.lights[0].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[0].extent = glm::vec2{ 1.f, 1.f };
	m_DeferredLights.lights[0].lightType = LightType::SPHERICAL_LIGHT;

    m_DeferredLights.lights[0].position = glm::vec4(0.0f, 10.0f, 15.0f, 0.0f);
	m_DeferredLights.lights[0].color = glm::vec3(0.f, 1.f, 0.f);
	m_DeferredLights.lights[0].radiance = 10.0f;
	m_DeferredLights.lights[0].radius = 1.f;
	m_DeferredLights.lights[0].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[0].extent = glm::vec2{ 1.f, 1.f };
	m_DeferredLights.lights[0].lightType = LightType::SPHERICAL_LIGHT;
	
	m_DeferredLights.lights[1].position = glm::vec4(0.0f, 10.0f, 10.0f, 0.0f);
	m_DeferredLights.lights[1].color = glm::vec3(1.f, 0.0f, 0.1f);
	m_DeferredLights.lights[1].radiance = 10.0f;
	m_DeferredLights.lights[1].radius = 0.0f;
	m_DeferredLights.lights[1].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[1].extent = glm::vec2{ 0.f, 0.f };
	m_DeferredLights.lights[1].lightType = LightType::POINT_LIGHT;
	
	m_DeferredLights.lights[2].position = glm::vec4(0.0f, 20.0f, 0.0f, 0.0f);
	m_DeferredLights.lights[2].color = glm::vec3(1.f, 1.f, 1.f);
	m_DeferredLights.lights[2].radiance = 5.0f;
	m_DeferredLights.lights[2].radius = 2.0f;
	m_DeferredLights.lights[2].normal = glm::vec3(0.f, 1.f, 0.f);
	m_DeferredLights.lights[2].extent = glm::vec2{ 80.f, 80.f };
	m_DeferredLights.lights[2].lightType = LightType::POINT_LIGHT;
	
	m_DeferredLights.lights[3].position = glm::vec4(0.0f, 40.0f, 10.0f, 0.0f);
	m_DeferredLights.lights[3].color = glm::vec3(1.f, 1.f, 1.f);
	m_DeferredLights.lights[3].radiance = 0.0f;
	m_DeferredLights.lights[3].radius = 1.0f;
	m_DeferredLights.lights[3].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[3].extent = glm::vec2{ 20.f, 20.f };
	m_DeferredLights.lights[3].lightType = LightType::AREA_LIGHT;
	
	m_DeferredLights.lights[4].position = glm::vec4(0.0f, 10.0f, 10.0f, 0.0f);
	m_DeferredLights.lights[4].color = glm::vec3(0.f, 0.f, 1.f);
	m_DeferredLights.lights[4].radiance = 20.0f;
	m_DeferredLights.lights[4].radius = 1.0f;
	m_DeferredLights.lights[4].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[4].extent = glm::vec2{ 3.f, 3.f };
	m_DeferredLights.lights[4].lightType = LightType::AREA_LIGHT;
	
	m_DeferredLights.lights[5].position = glm::vec4(0.0f, 40.0f, 10.0f, 0.0f);
	m_DeferredLights.lights[5].color = glm::vec3(1.f, 1.f, 1.f);
	m_DeferredLights.lights[5].radiance = 0.0f;
	m_DeferredLights.lights[5].radius = 1.0f;
	m_DeferredLights.lights[5].normal = glm::vec3(0.f, 0.f, -1.f);
	m_DeferredLights.lights[5].extent = glm::vec2{ 20.f, 20.f };
	m_DeferredLights.lights[5].lightType = LightType::AREA_LIGHT;
	
	m_DeferredLights.lights[0].position.x = 15 * cos(m_ElapsedSec/10.f);
	m_DeferredLights.lights[0].position.z = 15 * sin(m_ElapsedSec/10.f);
	
	m_DeferredLights.lights[1].position.x = 15 * cos(m_ElapsedSec + 1.5f);
	m_DeferredLights.lights[1].position.z = 15 * sin(m_ElapsedSec + 1.5f);
	
	// Current view position
	m_DeferredLights.viewPos = glm::vec4(m_Camera.GetPosition(), 1.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	m_DeferredLights.currentFrame = (++m_DeferredLights.currentFrame) % m_NumberOfPrevFrames;

	m_pLightBuffer->Update(&m_DeferredLights, sizeof(m_DeferredLights), GetCommandPool());
	
}
