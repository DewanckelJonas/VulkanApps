#pragma once
#include "Platform.h"
#include <vector>
namespace vkw
{
	class VulkanDevice;
	class Window;
	class Subpass;
	class RenderPass
	{
	public:
		RenderPass(VulkanDevice* pDevice, Window* pWindow, const std::vector<VkAttachmentDescription>& attachments, std::vector<VkSubpassDescription> subpassDescriptions, std::vector<VkSubpassDependency> dependencies);
		~RenderPass();

		VkRenderPass GetHandle();

	private:
		void Init();
		void Cleanup();

		VulkanDevice*									m_pDevice = nullptr;
		Window*											m_pWindow = nullptr;
		std::vector<VkAttachmentDescription>			m_AttachmentDescriptions{};
		std::vector<VkSubpassDescription>				m_SubPasses{};
		std::vector<VkSubpassDependency>				m_SubPassDependencies{};

		VkRenderPass									m_RenderPass = VK_NULL_HANDLE;
	};
}

