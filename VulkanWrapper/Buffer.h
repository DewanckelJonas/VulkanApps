#pragma once
#include "Platform.h"

namespace vkw
{
	class VulkanDevice;
	class CommandPool;
	class Buffer
	{
	public:
		Buffer(VulkanDevice* pDevice, CommandPool* cmdPool, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size,  void const* data);
		~Buffer();

		void Update(void const* data, size_t size, CommandPool* pCommandPool);
		const VkBuffer& GetHandle() const;
		VkDescriptorBufferInfo GetDescriptor() const;
		void Map();
		void UnMap();
		void* GetMappedMemory();
		static void CopyBuffer(CommandPool* pCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size);

	private:
		void Init(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags, size_t size, void const* data, CommandPool* cmdPool);
		void Cleanup();
		void UpdateDescriptor();

		VulkanDevice*						m_pDevice = nullptr;
		VkBuffer							m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory						m_Memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo				m_Descriptor{};
		VkDeviceSize						m_Size{};
		bool								m_UsingStagingBuffer{ false };
		void*								m_MappedMemory = nullptr;
	};
}

