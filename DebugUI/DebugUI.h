#pragma once
#include "VulkanWrapper/Platform.h"
#include <vector>
namespace vkw
{
	class VulkanDevice;
	class Window;
	class CommandPool;
	class DescriptorPool;
	class VulkanSwapchain;
	class RenderPass;
	class FrameBuffer;
	class DepthStencilBuffer;
	class DebugWindow;

	class DebugUI
	{
	public:
		DebugUI(VulkanDevice* pDevice, CommandPool* pCommandPool, Window* pWindow, VulkanSwapchain* pSwapchain, DepthStencilBuffer* pDepthStencilBuffer);
		~DebugUI();

		void NewFrame();
		void Render(VkSemaphore readyToRenderUISemaphore, FrameBuffer* pFrameBuffer, const std::vector<DebugWindow*>& pWindows);
		VkSemaphore GetDebugRenderCompleteSemaphore();

	private:
		void Init();
		void Cleanup();

		VulkanDevice*			m_pDevice = nullptr;
		Window*					m_pWindow = nullptr;
		CommandPool*			m_pCommandPool = nullptr;
		VulkanSwapchain*		m_pSwapchain = nullptr;
		RenderPass*				m_pRenderPass = nullptr;
		DepthStencilBuffer*		m_pDepthStencilBuffer = nullptr;
		VkDescriptorPool		m_DescriptorPool = VK_NULL_HANDLE;
		VkSemaphore				m_DebugRenderCompleteSemaphore = VK_NULL_HANDLE;
	};
}

