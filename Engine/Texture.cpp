#include "Texture.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include "CommandPool.h"
#include <algorithm>
using namespace vkw;

Texture::Texture(VulkanDevice* pDevice, CommandPool* cmdPool, TextureProperties properties, void* data, uint32_t width, uint32_t height, uint32_t layers)
	:m_ImageLayout(properties.imageLayout), m_Format(properties.format), m_pDevice(pDevice), m_Width(width), m_Height(height), m_Layers(layers)
{
	Init(cmdPool, properties, data);
}


Texture::~Texture()
{
	Cleanup();
}

VkDescriptorImageInfo vkw::Texture::GetDescriptor()
{
	return m_Descriptor;
}

VkImage vkw::Texture::GetImage()
{
	return m_Image;
}

VkImageView vkw::Texture::GetImageView()
{
	return m_ImageView;
}

VkImageLayout vkw::Texture::GetImageLayout()
{
	return m_ImageLayout;
}

VkFormat vkw::Texture::GetFormat()
{
	return m_Format;
}

uint32_t vkw::Texture::GetWidth()
{
	return m_Width;
}

uint32_t vkw::Texture::GetHeight()
{
	return m_Height;
}

uint32_t vkw::Texture::GetLayers()
{
	return m_Layers;
}


void vkw::Texture::Init(CommandPool* cmdPool, TextureProperties properties, void* data)
{
	if (data != nullptr) {

		properties.imageLayout = VkImageLayout(properties.imageLayout | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		properties.usageFlags = properties.usageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	CreateImage(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(), m_Width, m_Height, properties.format, VK_IMAGE_TILING_OPTIMAL, properties.usageFlags, properties.memFlags, m_Image, m_DeviceMemory, m_Layers);



	if(data != nullptr)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VkDeviceSize imageSize = m_Width * m_Height;

		CreateBuffer(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDeviceMemoryProperties(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* mappedMemory;
		vkMapMemory(m_pDevice->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedMemory);
		memcpy(mappedMemory, data, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_pDevice->GetDevice(), stagingBufferMemory);

		TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, properties.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), stagingBuffer, m_Image, m_Width, m_Height);

		vkDestroyBuffer(m_pDevice->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_pDevice->GetDevice(), stagingBufferMemory, nullptr);

		TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, properties.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_ImageLayout);
	}else
	{
		TransitionImageLayout(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool->GetHandle(), m_Image, properties.format, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout, m_Layers);
	}


	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	ErrorCheck(vkCreateSampler(m_pDevice->GetDevice(), &samplerCreateInfo, nullptr, &m_Sampler));

	
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	if(m_Layers > 1)
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	imageViewCreateInfo.format = properties.format;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageViewCreateInfo.subresourceRange.layerCount = m_Layers;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.image = m_Image;
	ErrorCheck(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCreateInfo, nullptr, &m_ImageView));

	UpdateDescriptor();

}

void vkw::Texture::Cleanup()
{
	vkDestroySampler(m_pDevice->GetDevice(), m_Sampler, nullptr);
	vkDestroyImageView(m_pDevice->GetDevice(), m_ImageView, nullptr);
	vkDestroyImage(m_pDevice->GetDevice(), m_Image, nullptr);
	vkFreeMemory(m_pDevice->GetDevice(), m_DeviceMemory, nullptr);
}

void vkw::Texture::CopyTo(Texture * texture, VkCommandPool cmdPool, uint32_t sourceLayer, uint32_t destLayer)
{
	if((texture->GetImageLayout() & VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) == false)
	{
		assert("Cant copy to image that does not have transfer destination bit!" && 0);
		std::exit(-1);
	}

	if (m_Width != texture->GetWidth() || m_Height != texture->GetHeight())
	{
		assert("Cant copy to image with different size!" && 0);
		std::exit(-1);
	}

	VkCommandBuffer cmdBuffer = BeginSingleTimeCommands(m_pDevice->GetDevice(), cmdPool);
	//VkImageBlit imageBlit{};

	VkImageCopy imageCopy{};
	imageCopy.dstOffset = { 0, 0, 0 };
	imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopy.dstSubresource.baseArrayLayer = destLayer;
	imageCopy.dstSubresource.layerCount = 1;
	imageCopy.dstSubresource.mipLevel = 0;
	imageCopy.extent.width = m_Width;
	imageCopy.extent.height = m_Height;
	imageCopy.extent.depth = 1;
	imageCopy.srcOffset = { 0, 0, 0 };
	imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopy.srcSubresource.baseArrayLayer = sourceLayer;
	imageCopy.srcSubresource.layerCount = 1;
	imageCopy.srcSubresource.mipLevel = 0;

	vkCmdCopyImage(cmdBuffer, m_Image, m_ImageLayout, texture->GetImage(), texture->GetImageLayout(), 1, &imageCopy);
	//vkCmdBlitImage(cmdBuffer, m_Image, m_ImageLayout, texture->GetImage(), texture->GetImageLayout(), 1, imageBlit, VK_FILTER_LINEAR)

	EndSingleTimeCommands(m_pDevice->GetDevice(), m_pDevice->GetQueue(), cmdPool, cmdBuffer);
}

void Texture::UpdateDescriptor()
{
	m_Descriptor.sampler = m_Sampler;
	m_Descriptor.imageView = m_ImageView;
	m_Descriptor.imageLayout = m_ImageLayout;
}
