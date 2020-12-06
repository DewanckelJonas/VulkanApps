#include "CommandPool.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"

using namespace vkw;



CommandPool::CommandPool(VulkanDevice* pDevice, VkCommandPoolCreateFlags flags, uint32_t familyQueueId)
	:m_pDevice(pDevice), m_Flags(flags), m_FamilyQueueId(familyQueueId)
{
	Init();
}

CommandPool::~CommandPool()
{
	Cleanup();
}

void vkw::CommandPool::Reset(bool releaseResources)
{
	VkCommandPoolResetFlags resetFlags{};
	if (releaseResources)
		resetFlags = VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
	ErrorCheck(vkResetCommandPool(m_pDevice->GetDevice(), m_CommandPool, resetFlags));
}

std::vector<VkCommandBuffer> CommandPool::CreateCommandBuffers(size_t amount, VkCommandBufferLevel bufferLevel)
{
	std::vector<VkCommandBuffer> commandBuffers{amount};

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_CommandPool;
	commandBufferAllocateInfo.level = bufferLevel;
	commandBufferAllocateInfo.commandBufferCount = uint32_t(amount);

	ErrorCheck(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &commandBufferAllocateInfo, commandBuffers.data()));

	return commandBuffers;
}

VkCommandBuffer CommandPool::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	ErrorCheck(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &allocInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	ErrorCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	return commandBuffer;
}

void CommandPool::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkSemaphore signalSemaphore, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage)
{
	ErrorCheck(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	if(signalSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
	}
	if(waitSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
	}
	submitInfo.pWaitDstStageMask = &waitStage;


	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	ErrorCheck(vkCreateFence(m_pDevice->GetDevice(), &fenceInfo, nullptr, &fence));

	ErrorCheck(vkQueueSubmit(m_pDevice->GetQueue(), 1, &submitInfo, fence));

	// Wait for the fence to signal that command buffer has finished executing
	ErrorCheck(vkWaitForFences(m_pDevice->GetDevice(), 1, &fence, VK_TRUE, 1000000000));
	vkFreeCommandBuffers(m_pDevice->GetDevice(), m_CommandPool, 1, &commandBuffer);

	vkDestroyFence(m_pDevice->GetDevice(), fence, nullptr);
}

VkCommandPool vkw::CommandPool::GetHandle()
{
	return m_CommandPool;
}

void CommandPool::Init()
{
	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = m_Flags;
	poolCreateInfo.queueFamilyIndex = m_FamilyQueueId;
	ErrorCheck(vkCreateCommandPool(m_pDevice->GetDevice(), &poolCreateInfo, nullptr, &m_CommandPool));
}

void vkw::CommandPool::Cleanup()
{
	vkDestroyCommandPool(m_pDevice->GetDevice(), m_CommandPool, nullptr);
}

