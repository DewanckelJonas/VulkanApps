#pragma once
#include "glm/glm.hpp"
#include <array>
enum VkResult;
void ErrorCheck(VkResult result);
void CreateBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties deviceMemoryProperties, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
void CreateImage(VkDevice device, const VkPhysicalDeviceMemoryProperties & physicalDeviceMemProperties, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory, uint32_t arrayLayers = 1, uint32_t mipLevels = 1, VkImageCreateFlags flags = 0);
void TransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t arrayLayers = 1, uint32_t mipLevels = 1);
void CopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool cmdPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format);
uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties* gpuMemoryProperties, const VkMemoryRequirements* memoryRequirements, const VkMemoryPropertyFlags memoryProperties);
struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct Circle
{
	glm::vec3 center;
	float radius;
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};