#pragma once
#include "Buffer.h"
namespace vkw
{
	class VulkanDevice;
	class CommandPool;
	class IndexBuffer
	{
	public:
		IndexBuffer(VulkanDevice* pDevice, CommandPool* cmdPool, size_t size, uint32_t const* data)
			:m_Buffer(pDevice, cmdPool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size*sizeof(uint32_t), data)
		{
			m_IndexCount = size;
		}
		const Buffer& GetBuffer() { return m_Buffer; }
		size_t GetIndexCount() { return m_IndexCount; }

	private:
		Buffer			m_Buffer;
		size_t			m_IndexCount = 0;
	};
}