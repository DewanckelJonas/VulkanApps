#pragma once
#include "Platform.h"
#include <string>
#include <glm/glm.hpp>

struct GLFWwindow;
namespace vkw 
{
	class VulkanDevice;
	class VulkanBaseApp;
	class Window
	{
	public:
		Window(VulkanBaseApp* app, VulkanDevice* device, uint32_t width, uint32_t height);
		~Window();
		//void Update();
		void UpdateSurfaceSize();
		//bool IsKeyButtonDown(int key);
		//glm::vec2 GetMousePos();

		VkSurfaceKHR GetSurface();
		VkExtent2D GetSurfaceSize();
		VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
		VkSurfaceFormatKHR GetSurfaceFormat();

	private:

		void Init();
		void InitSurface();
		void CleanupSurface();
		void Cleanup();

		VulkanBaseApp*						m_pApp = nullptr;
		VulkanDevice*						m_pDevice = nullptr;

		VkSurfaceKHR						m_Surface = VK_NULL_HANDLE;
		VkExtent2D							m_SurfaceSize{};
		VkSurfaceCapabilitiesKHR			m_SurfaceCapabilities{};
		VkSurfaceFormatKHR					m_SurfaceFormat{};
		GLFWwindow*							m_pWindow;
	};
}

