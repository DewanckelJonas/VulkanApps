#pragma once
#include "Platform.h"
#include <string>
#include <vector>

namespace vkw
{
	class VulkanDevice;
	class Window;
	class VulkanSwapchain;
	class DepthStencilBuffer;
	class RenderPass;
	class FrameBuffer;
	class CommandPool;
	class VulkanBaseApp
	{
	public:
		VulkanBaseApp(VulkanDevice* pDevice, const std::string& appName);
		~VulkanBaseApp();

		virtual void Render() {};
		virtual bool Update(float dTime);
		virtual void Init(uint32_t width, uint32_t height);
		virtual void Cleanup();
		void Close();
		virtual void Resize();

		std::string GetName();


	protected:
		void InitWindow(uint32_t width, uint32_t height);
		void InitSynchronizations();
		void InitSwapchain(VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR, uint32_t swapchainImageCount = 2);
		void InitDepthStencilBuffer();
		void InitRenderPass();
		void InitFramebuffers();
		void InitPipelineCache();
		void InitCommandPool(VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		void AllocateDrawCommandBuffers();

		void CleanupWindow();
		void CleanupSynchronizations();
		void CleanupSwapchain();
		void CleanupDepthStencilBuffer();
		void CleanupRenderPass();
		void CleanupFramebuffers();
		void CleanupPipelineCache();
		void CleanupCommandPool();
		void FreeDrawCommandBuffers();

		bool IsRunning();

		VulkanDevice* GetDevice();
		VulkanSwapchain* GetSwapchain();
		Window* GetWindow();
		RenderPass* GetRenderPass();
		CommandPool* GetCommandPool();
		const VkSemaphore& GetPresentCompleteSemaphore();
		const VkSemaphore& GetRenderCompleteSemaphore();
		const std::vector<VkFence>& GetWaitFences();
		const std::vector<VkCommandBuffer>& GetDrawCommandBuffers();
		const std::vector<FrameBuffer*>& GetFrameBuffers();
		VkPipelineCache GetPipelineCache();

	private:
		VulkanDevice*					m_pDevice = nullptr;
		std::string						m_AppName{};
		bool							m_IsRunning{ true };
		bool							m_IsInitialized{ false };
		Window*							m_pWindow = nullptr;
		VulkanSwapchain*				m_pSwapchain = nullptr;
		DepthStencilBuffer*				m_pDepthStencilBuffer = nullptr;
		RenderPass*						m_pRenderPass = nullptr;
		std::vector<FrameBuffer*>		m_FrameBuffers{};
		VkPipelineCache					m_PipelineCache = VK_NULL_HANDLE;
		CommandPool*					m_pCommandPool = nullptr;
		VkSemaphore						m_PresentCompleteSemaphore = VK_NULL_HANDLE;
		VkSemaphore						m_RenderCompleteSemaphore = VK_NULL_HANDLE;
		std::vector<VkFence>			m_WaitFences{};
		std::vector<VkCommandBuffer>	m_DrawCommandBuffers{};
	};
}

