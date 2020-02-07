#include "VulkanBaseApp.h"
#include "Window.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include "DepthStencilBuffer.h"
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "CommandPool.h"

using namespace vkw;

VulkanBaseApp::VulkanBaseApp(VulkanDevice * pDevice, const std::string & appName)
	:m_pDevice(pDevice), m_AppName(appName)
{
}

VulkanBaseApp::~VulkanBaseApp()
{
}

bool vkw::VulkanBaseApp::Update(float)
{
	m_pWindow->Update();
	return m_IsRunning;
}

void VulkanBaseApp::Init(uint32_t width, uint32_t height)
{
	m_pDevice->Init();
	InitWindow(width, height);
	InitSwapchain();
	InitSynchronizations();
	InitDepthStencilBuffer();
	InitRenderPass();
	InitPipelineCache();
	InitFramebuffers();
	InitCommandPool();
	AllocateDrawCommandBuffers();
	m_IsInitialized = true;
	return;
}

void VulkanBaseApp::Close()
{
	m_IsRunning = false;
}

void vkw::VulkanBaseApp::Resize()
{
	if (!m_IsInitialized)
		return;
	ErrorCheck(vkDeviceWaitIdle(m_pDevice->GetDevice()));

	m_pWindow->UpdateSurfaceSize();

	FreeDrawCommandBuffers();
	CleanupCommandPool();
	CleanupRenderPass();
	CleanupFramebuffers();
	CleanupDepthStencilBuffer();
	CleanupSwapchain();


	InitSwapchain();
	InitDepthStencilBuffer();
	InitRenderPass();
	InitFramebuffers();
	InitCommandPool();
	AllocateDrawCommandBuffers();
}

std::string VulkanBaseApp::GetName()
{
	return m_AppName;
}

void VulkanBaseApp::InitWindow(uint32_t width, uint32_t height)
{
	m_pWindow = new Window(this, m_pDevice, width, height);
}

void VulkanBaseApp::InitSynchronizations()
{
	//Semaphores
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	ErrorCheck(vkCreateSemaphore(m_pDevice->GetDevice(), &semaphoreCreateInfo, nullptr, &m_PresentCompleteSemaphore));

	ErrorCheck(vkCreateSemaphore(m_pDevice->GetDevice(), &semaphoreCreateInfo, nullptr, &m_RenderCompleteSemaphore));

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	m_WaitFences.resize(m_pSwapchain->GetImageCount());
	for (size_t i = 0; i < m_WaitFences.size(); i++)
	{
		ErrorCheck(vkCreateFence(m_pDevice->GetDevice(), &fenceCreateInfo, nullptr, &m_WaitFences[i]));
	}
}

void VulkanBaseApp::InitSwapchain(VkPresentModeKHR preferredPresentMode, uint32_t swapchainImageCount)
{
	m_pSwapchain = new VulkanSwapchain(m_pDevice, m_pWindow, preferredPresentMode, swapchainImageCount);
}

void VulkanBaseApp::InitDepthStencilBuffer()
{
	m_pDepthStencilBuffer = new DepthStencilBuffer(m_pDevice, m_pWindow);
}

void VulkanBaseApp::InitRenderPass()
{
	std::vector<VkAttachmentDescription> attachments{2};
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[0].format = m_pDepthStencilBuffer->GetFormat();

	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
	m_pRenderPass = new RenderPass(m_pDevice, m_pWindow, attachments, subPasses, dependencies);
}

void VulkanBaseApp::InitFramebuffers()
{
	m_FrameBuffers.resize(m_pSwapchain->GetImageCount());
	std::vector<VkImageView> swapchainImageViews = m_pSwapchain->GetImageViews();
	for (size_t i = 0; i < m_FrameBuffers.size(); i++)
	{
		std::vector<VkImageView> attachments{ m_pDepthStencilBuffer->GetImageView(), swapchainImageViews[i] };
		m_FrameBuffers[i] = new FrameBuffer(m_pDevice, m_pRenderPass, m_pWindow, attachments);
	}
}

void vkw::VulkanBaseApp::InitPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ErrorCheck(vkCreatePipelineCache(m_pDevice->GetDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
}

void vkw::VulkanBaseApp::InitCommandPool(VkCommandPoolCreateFlags flags)
{
	m_pCommandPool = new CommandPool(m_pDevice, flags, m_pDevice->GetGraphicsFamilyQueueId());
}

void vkw::VulkanBaseApp::AllocateDrawCommandBuffers()
{
	m_DrawCommandBuffers.resize(m_pSwapchain->GetImageCount());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_pCommandPool->GetHandle();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = uint32_t(m_DrawCommandBuffers.size());

	ErrorCheck(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers.data()));
}

void VulkanBaseApp::Cleanup()
{
	CleanupPipelineCache();
	FreeDrawCommandBuffers();
	CleanupCommandPool();
	CleanupRenderPass();
	CleanupFramebuffers();
	CleanupDepthStencilBuffer();
	CleanupSynchronizations();
	CleanupSwapchain();
	CleanupWindow();
	return;
}

void VulkanBaseApp::CleanupWindow()
{
	delete m_pWindow;
}

void vkw::VulkanBaseApp::CleanupSynchronizations()
{
	vkDestroySemaphore(m_pDevice->GetDevice(), m_PresentCompleteSemaphore, nullptr);
	vkDestroySemaphore(m_pDevice->GetDevice(), m_RenderCompleteSemaphore, nullptr);
	for (size_t i = 0; i < m_WaitFences.size(); i++)
	{
		vkDestroyFence(m_pDevice->GetDevice(), m_WaitFences[i], nullptr);
	}
}

void VulkanBaseApp::CleanupSwapchain()
{
	delete m_pSwapchain;
}

void VulkanBaseApp::CleanupDepthStencilBuffer()
{
	delete m_pDepthStencilBuffer;
}

void VulkanBaseApp::CleanupRenderPass()
{
	delete m_pRenderPass;
}

void VulkanBaseApp::CleanupFramebuffers()
{
	for (size_t i = 0; i < m_FrameBuffers.size(); i++)
	{
		delete m_FrameBuffers[i];
	}
}

void vkw::VulkanBaseApp::CleanupPipelineCache()
{
	vkDestroyPipelineCache(m_pDevice->GetDevice(), m_PipelineCache, nullptr);
}

void vkw::VulkanBaseApp::CleanupCommandPool()
{
	delete m_pCommandPool;
}

void vkw::VulkanBaseApp::FreeDrawCommandBuffers()
{
	vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pCommandPool->GetHandle(), uint32_t(m_DrawCommandBuffers.size()), m_DrawCommandBuffers.data());
}

bool vkw::VulkanBaseApp::IsRunning()
{
	return m_IsRunning;
}

VulkanDevice * VulkanBaseApp::GetDevice()
{
	return m_pDevice;
}

VulkanSwapchain * vkw::VulkanBaseApp::GetSwapchain()
{
	return m_pSwapchain;
}

Window * vkw::VulkanBaseApp::GetWindow()
{
	return m_pWindow;
}

RenderPass * vkw::VulkanBaseApp::GetRenderPass()
{
	return m_pRenderPass;
}

CommandPool * vkw::VulkanBaseApp::GetCommandPool()
{
	return m_pCommandPool;
}

const VkSemaphore& vkw::VulkanBaseApp::GetPresentCompleteSemaphore()
{
	return m_PresentCompleteSemaphore;
}

const VkSemaphore& vkw::VulkanBaseApp::GetRenderCompleteSemaphore()
{
	return m_RenderCompleteSemaphore;
}

const std::vector<VkFence>& vkw::VulkanBaseApp::GetWaitFences()
{
	return m_WaitFences;
}

const std::vector<VkCommandBuffer>& vkw::VulkanBaseApp::GetDrawCommandBuffers()
{
	return m_DrawCommandBuffers;
}

const std::vector<FrameBuffer*>& vkw::VulkanBaseApp::GetFrameBuffers()
{
	return m_FrameBuffers;
}

VkPipelineCache vkw::VulkanBaseApp::GetPipelineCache()
{
	return m_PipelineCache;
}

