#pragma once
#include "Platform.h"
#include <string>
#include <glm/glm.hpp>
#include <functional>
#include <vector>
#include "MouseButtonCodes.h"

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
		bool Update();
		void UpdateSurface();
		bool IsKeyPressed(int key);
		bool IsKeyReleased(int key);
		bool IsMouseButtonPressed(MouseButton button);
		bool IsMouseButtonReleased(MouseButton button);
		glm::vec2 GetMousePos();

		void WaitOnEvents();
		void AddOnResizeCallback(const std::function<void(Window*)>& pWindow);

		VkSurfaceKHR GetSurface();
		VkExtent2D GetSurfaceSize();
		VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
		VkSurfaceFormatKHR GetSurfaceFormat();
		GLFWwindow* GetWindowPtr();

	private:
		void Init();
		void InitSurface();
		void CleanupSurface();
		void Cleanup();
		void OnWindowResize(GLFWwindow* pWindow, int width, int height);
		static void OnWindowResizeCallback(GLFWwindow* pWindow, int width, int height);

		VulkanBaseApp*						m_pApp = nullptr;
		VulkanDevice*						m_pDevice = nullptr;

		VkSurfaceKHR						m_Surface = VK_NULL_HANDLE;
		VkExtent2D							m_SurfaceSize{};
		VkSurfaceCapabilitiesKHR			m_SurfaceCapabilities{};
		VkSurfaceFormatKHR					m_SurfaceFormat{};
		GLFWwindow*							m_pWindow = nullptr;

		std::vector<std::function<void(Window*)>> m_WindowResizeCallbacks{};
	};
}

