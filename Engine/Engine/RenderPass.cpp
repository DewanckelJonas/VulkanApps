#include "RenderPass.h"
#include "VulkanDevice.h"
#include "DepthStencilBuffer.h"
#include "Window.h"
#include "VulkanHelpers.h"
#include <vector>

using namespace vkw;

RenderPass::RenderPass(VulkanDevice* pDevice, Window* pWindow, const std::vector<VkAttachmentDescription>& attachments, std::vector<VkSubpassDescription> subpassDescriptions, std::vector<VkSubpassDependency> dependencies)
	:m_pDevice(pDevice), m_pWindow(pWindow), m_AttachmentDescriptions(attachments), m_SubPasses(subpassDescriptions), m_SubPassDependencies(dependencies)
{
	Init();
}


RenderPass::~RenderPass()
{
	Cleanup();
}

VkRenderPass vkw::RenderPass::GetHandle()
{
	return m_RenderPass;
}

void RenderPass::Init()
{
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = uint32_t(m_AttachmentDescriptions.size());
	renderPassCreateInfo.pAttachments = m_AttachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = uint32_t(m_SubPasses.size());
	renderPassCreateInfo.pSubpasses = m_SubPasses.data();
	renderPassCreateInfo.dependencyCount = uint32_t(m_SubPassDependencies.size());
	renderPassCreateInfo.pDependencies = m_SubPassDependencies.data();

	ErrorCheck(vkCreateRenderPass(m_pDevice->GetDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

void vkw::RenderPass::Cleanup()
{
	vkDestroyRenderPass(m_pDevice->GetDevice(), m_RenderPass, nullptr);
}
