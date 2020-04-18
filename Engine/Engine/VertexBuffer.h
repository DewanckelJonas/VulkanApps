#pragma once
#include "VertexLayout.h"
#include "Buffer.h"
namespace vkw
{
	class VulkanDevice;
	class CommandPool;
	class VertexBuffer
	{
	public:
		VertexBuffer(VulkanDevice* pDevice, CommandPool* cmdPool, const VertexLayout& layout, size_t size, void* data)
			: m_Layout(layout)
			, m_Buffer(pDevice, cmdPool, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size, data)
		{}
		Buffer& GetBuffer() { return m_Buffer; } 
		VertexLayout& GetLayout() { return m_Layout; } 


	private:
		VertexLayout	m_Layout;
		Buffer			m_Buffer;
	};
}

