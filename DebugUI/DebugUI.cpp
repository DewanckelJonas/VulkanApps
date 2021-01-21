#include "DebugUI.h"
#include <imgui.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "VulkanWrapper/VulkanDevice.h"
#include "VulkanWrapper/Window.h"
#include "VulkanWrapper/VulkanHelpers.h"
#include "VulkanWrapper/VulkanSwapchain.h"
#include "VulkanWrapper/RenderPass.h"
#include "VulkanWrapper/CommandPool.h"
#include "VulkanWrapper/FrameBuffer.h"
#include "VulkanWrapper/DepthStencilBuffer.h"
#include "DebugWindow.h"



vkw::DebugUI::DebugUI(VulkanDevice* pDevice, CommandPool* pCommandPool, Window* pWindow, VulkanSwapchain* pSwapchain, DepthStencilBuffer* pDepthStencilBuffer)
	:m_pDevice{pDevice}
	,m_pWindow{pWindow}
	,m_pSwapchain{pSwapchain}
	,m_pCommandPool{pCommandPool}
	,m_pDepthStencilBuffer{pDepthStencilBuffer}
{
	Init();
}

vkw::DebugUI::~DebugUI()
{
	Cleanup();
}

void vkw::DebugUI::NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void vkw::DebugUI::Render(VkSemaphore readyToRenderUISemaphore, FrameBuffer* pFramebuffer, const std::vector<DebugWindow*>& pWindows)
{
	for (DebugWindow* pWindow : pWindows)
	{
		pWindow->Render();
	}
	
	ImGui::ShowDemoWindow();

	ImGui::Render();

	//m_pCommandPool->Reset();
	VkCommandBuffer commandBuffer = m_pCommandPool->BeginSingleTimeCommands();

	VkClearValue clearValues[2];
	clearValues[1].color = { 0.5f, 0.5f, 0.5f, 0.f };
	clearValues[0].depthStencil = { 0.f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_pRenderPass->GetHandle();
	renderPassBeginInfo.framebuffer = pFramebuffer->GetHandle();
	renderPassBeginInfo.renderArea.extent = m_pWindow->GetSurfaceSize();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Record Imgui Draw Data and draw funcs into renderpass
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
	m_pCommandPool->EndSingleTimeCommands(commandBuffer, m_DebugRenderCompleteSemaphore, readyToRenderUISemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

VkSemaphore vkw::DebugUI::GetDebugRenderCompleteSemaphore()
{
	return m_DebugRenderCompleteSemaphore;
}

void vkw::DebugUI::Init()
{
	//Create Renderpass
	std::vector<VkAttachmentDescription> attachments{ 2 };
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[0].format = m_pDepthStencilBuffer->GetFormat();

	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[1].format = m_pWindow->GetSurfaceFormat().format;


	VkAttachmentReference subPass0DepthStencilAttachment{};
	subPass0DepthStencilAttachment.attachment = 0;
	subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> subPass0ColorAttachments{};
	subPass0ColorAttachments[0].attachment = 1; //this int is the index of the attachments passed to the renderPass.
	subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkSubpassDescription> subPasses{ 1 };
	subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPasses[0].colorAttachmentCount = uint32_t(subPass0ColorAttachments.size());
	subPasses[0].pColorAttachments = subPass0ColorAttachments.data();
	subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;

	std::vector<VkSubpassDependency> dependencies{ 1 };
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	m_pRenderPass = new RenderPass(m_pDevice, attachments, subPasses, dependencies);

	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize.descriptorCount = 1;
	
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;
	descriptorPoolInfo.maxSets = uint32_t(m_pSwapchain->GetImageCount());

	ErrorCheck(vkCreateDescriptorPool(m_pDevice->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	io.Fonts->AddFontDefault();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(m_pWindow->GetWindowPtr(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_pDevice->GetInstance();
	init_info.PhysicalDevice = m_pDevice->GetPhysicalDevice();
	init_info.Device = m_pDevice->GetDevice();
	init_info.QueueFamily = m_pDevice->GetGraphicsFamilyQueueId();
	init_info.Queue = m_pDevice->GetQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = m_DescriptorPool;
	init_info.Allocator = VK_NULL_HANDLE;
	init_info.MinImageCount = uint32_t(m_pSwapchain->GetImageCount());
	init_info.ImageCount = uint32_t(m_pSwapchain->GetImageCount());
	init_info.CheckVkResultFn = ErrorCheck;
	ImGui_ImplVulkan_Init(&init_info, m_pRenderPass->GetHandle());

	VkCommandBuffer cmdBuffer = m_pCommandPool->BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	m_pCommandPool->EndSingleTimeCommands(cmdBuffer);
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	ErrorCheck(vkCreateSemaphore(m_pDevice->GetDevice(), &semaphoreCreateInfo, nullptr, &m_DebugRenderCompleteSemaphore));
}

void vkw::DebugUI::Cleanup()
{
	ErrorCheck(vkDeviceWaitIdle(m_pDevice->GetDevice()));
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete m_pRenderPass;
	vkDestroyDescriptorPool(m_pDevice->GetDevice(), m_DescriptorPool, nullptr);
	vkDestroySemaphore(m_pDevice->GetDevice(), m_DebugRenderCompleteSemaphore, nullptr);
}

