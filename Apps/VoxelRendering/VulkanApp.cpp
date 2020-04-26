#include "VulkanApp.h"
#include "VulkanSwapchain.h"
#include "VulkanHelpers.h"
#include "Buffer.h"
#include "Texture.h"
#include "VulkanDevice.h"
#include "Shader.h"
#include <array>
#include "Window.h"
#include "RenderPass.h"
#include "CommandPool.h"
#include "FrameBuffer.h"
#include <sstream>
#include <algorithm>
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "DepthStencilBuffer.h"
#include "RaytracingGeometry.h"
#include "GraphicsPipeline.h"
#include "VertexBuffer.h"
#include <iostream>

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "VoxelTest")
{
}

VulkanApp::~VulkanApp()
{
}

void VulkanApp::Render()
{
	GetSwapchain()->AcquireNextImage(GetPresentCompleteSemaphore());

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &GetPresentCompleteSemaphore();
	VkPipelineStageFlags waitDstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitDstMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &GetRenderCompleteSemaphore();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &GetDrawCommandBuffers()[GetSwapchain()->GetActiveImageId()];

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));

	GetSwapchain()->PresentImage(GetRenderCompleteSemaphore());
}

bool VulkanApp::Update(float dTime)
{
	m_ElapsedSec += dTime;
	//glm::vec2 mousePos = GetWindow()->GetMousePos();
	//glm::vec2 mouseMovement = mousePos - m_PrevMousePos;
	//m_PrevMousePos = mousePos;
	//if(GetWindow()->IsKeyButtonDown(VK_LSHIFT))
	//{
	//	m_Camera.SetMovementSpeed(100.f);
	//}
	//else
	//{
	//	m_Camera.SetMovementSpeed(5.f);
	//}
	//Camera_Movement movementDir{Camera_Movement::NONE};
	//if(GetWindow()->IsKeyButtonDown('W')) movementDir = Camera_Movement::FORWARD;
	//if(GetWindow()->IsKeyButtonDown('S')) movementDir = Camera_Movement::BACKWARD;
	//if(GetWindow()->IsKeyButtonDown('A')) movementDir = Camera_Movement::LEFT;
	//if(GetWindow()->IsKeyButtonDown('D')) movementDir = Camera_Movement::RIGHT;
	//m_Camera.ProcessKeyboard(movementDir, dTime);
	//float sensitivity = 3.f;
	//if(GetWindow()->IsKeyButtonDown(VK_LBUTTON))
	//{
	//	m_Camera.ProcessMouseMovement(sensitivity * mouseMovement.x, sensitivity * mouseMovement.y, true);
	//}
	UpdateUniformBuffers();
	return VulkanBaseApp::Update(dTime);
}

void VulkanApp::Init(uint32_t width, uint32_t height)
{
	//EnableRaytracingExtension();
	VulkanBaseApp::Init(width, height);
	m_pUniformBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size_t(sizeof(CameraInfo)), &m_Ubo);
	CreateVertexBuffer();
	m_pDescriptorPool = new vkw::DescriptorPool(GetDevice());
	m_pDescriptorSet = new vkw::DescriptorSet();
	m_pDescriptorSet->AddBinding(m_pUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pDescriptorSet->AddBinding(m_pTerrainBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pDescriptorPool->AddDescriptorSet(m_pDescriptorSet);
	m_pDescriptorPool->Allocate();
	m_pGraphicsPipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pDescriptorSet->GetLayout(), &m_pVertexBuffer->GetLayout(), "../Shaders/Perspective.vert.spv", "../Shaders/Color.frag.spv", VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkFrontFace::VK_FRONT_FACE_CLOCKWISE);
	BuildDrawCommandBuffers();
	//m_PrevMousePos = GetWindow()->GetMousePos();
}

void VulkanApp::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	delete m_pGraphicsPipeline;
	delete m_pDescriptorPool;
	delete m_pUniformBuffer;
	delete m_pTerrainBuffer;
	delete m_pVertexBuffer;
	VulkanBaseApp::Cleanup();
}

void VulkanApp::EnableRaytracingExtension()
{
	GetDevice()->EnableInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);
}

void VulkanApp::CreateVertexBuffer()
{
	glm::vec3 vertices[] = {
	{-1.0f,-1.0f,-1.0f},
	{-1.0f,-1.0f, 1.0f },
	{-1.0f, 1.0f, 1.0f}, 
	{1.0f, 1.0f,-1.0f}, 
	{-1.0f,-1.0f,-1.0f},
	{-1.0f, 1.0f,-1.0f}, 
	{1.0f,-1.0f, 1.0f},
	{-1.0f,-1.0f,-1.0f},
	{1.0f,-1.0f,-1.0f},
	{1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f,-1.0f},
	{-1.0f,-1.0f,-1.0f},
	{-1.0f,-1.0f,-1.0f},
	{-1.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f, 1.0f},
	{-1.0f,-1.0f, 1.0},
	{-1.0f,-1.0f,-1.0},
	{-1.0f, 1.0f, 1.0},
	{-1.0f,-1.0f, 1.0},
	{1.0f,-1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},
	{1.0f,-1.0f,-1.0f},
	{1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f,-1.0f},
	{1.0f, 1.0f, 1.0f},
	{1.0f,-1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f,-1.0f},
	{-1.0f, 1.0f,-1.0},
	{1.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f,-1.0},
	{-1.0f, 1.0f, 1.0},
	{1.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f, 1.0},
	{1.0f,-1.0f, 1.0f}
	};

	bool Terrain[1000000]{ false };

	for (size_t y = 0; y < 100; y++)//collums // fill half of the rows
	{
		for (size_t z = 0; z < 100; z++)
		{
			for (size_t x = 0; x < 100; x++)
			{
				Terrain[y * 10000 + z * 100 + x] = true;
			}
		}
	}

	m_pTerrainBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1000000, Terrain);

	m_VertexCount = sizeof(vertices)/sizeof(glm::vec3);
	m_pVertexBuffer = new vkw::VertexBuffer(GetDevice(), GetCommandPool(), vkw::VertexLayout({ vkw::VertexLayout::Types::POSITION  }), sizeof(vertices), vertices);
}

void VulkanApp::BuildDrawCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkClearValue clearValues[2];
	clearValues[1].color = { 0.5f, 0.5f, 0.5f, 1.f };
	clearValues[0].depthStencil = { 10000.f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;;
	renderPassBeginInfo.renderPass = GetRenderPass()->GetHandle();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = GetWindow()->GetSurfaceSize();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	
	for (int32_t i = 0; i < GetDrawCommandBuffers().size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(GetDrawCommandBuffers()[i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(GetDrawCommandBuffers()[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.width = float(GetWindow()->GetSurfaceSize().width);
		viewport.height = -float(GetWindow()->GetSurfaceSize().height); //flip vulkan viewport so y is up
		viewport.x = 0.f;
		viewport.y = float(GetWindow()->GetSurfaceSize().height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(GetDrawCommandBuffers()[i], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = GetWindow()->GetSurfaceSize();
		scissor.offset = { 0, 0 };

		vkCmdSetScissor(GetDrawCommandBuffers()[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(GetDrawCommandBuffers()[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(GetDrawCommandBuffers()[i], 0, 1, &m_pVertexBuffer->GetBuffer().GetHandle(), offsets);
		vkCmdDraw(GetDrawCommandBuffers()[i], m_VertexCount, 1000000, 0, 0);
		//vkCmdBindIndexBuffer(GetDrawCommandBuffers()[i], m_pIndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		//vkCmdDrawIndexed(GetDrawCommandBuffers()[i], 6, 1, 0, 0, 1);


		vkCmdEndRenderPass(GetDrawCommandBuffers()[i]);

		ErrorCheck(vkEndCommandBuffer(GetDrawCommandBuffers()[i]));
	}
}

void VulkanApp::UpdateUniformBuffers()
{
	m_Ubo.projection = m_Camera.GetProjectionMatrix(float(GetWindow()->GetSurfaceSize().width), float(GetWindow()->GetSurfaceSize().height), 0.1f,  10000.f);
	m_Ubo.view = m_Camera.GetViewMatrix();
	m_pUniformBuffer->Update(&m_Ubo, sizeof(m_Ubo), GetCommandPool());
}












