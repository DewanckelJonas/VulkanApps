#include "VulkanApp.h"
#include "VulkanWrapper/VulkanSwapchain.h"
#include "VulkanWrapper/VulkanHelpers.h"
#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/Texture.h"
#include "VulkanWrapper/VulkanDevice.h"
#include "VulkanWrapper/Shader.h"
#include <array>
#include "VulkanWrapper/Window.h"
#include "VulkanWrapper/RenderPass.h"
#include "VulkanWrapper/CommandPool.h"
#include "VulkanWrapper/FrameBuffer.h"
#include <sstream>
#include <algorithm>
#include "VulkanWrapper/DescriptorPool.h"
#include "VulkanWrapper/DescriptorSet.h"
#include "VulkanWrapper/DepthStencilBuffer.h"
#include "VulkanWrapper/RaytracingGeometry.h"
#include "VulkanWrapper/GraphicsPipeline.h"
#include "VulkanWrapper/VertexBuffer.h"
#include <iostream>
#include <DebugUI/DebugUI.h>
#include <DebugUI/DebugWindow.h>
#include <DebugUI/DebugUIElements.h>
#include <chrono>
#include <DataHandling/MeshShapes.h>
#include <VulkanWrapper/IndexBuffer.h>
#include <Base/Array3D.h>
#include <DebugUI/DebugShaderEditor.h>

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "VoxelTest")
{
}

VulkanApp::~VulkanApp()
{
}

void VulkanApp::Render()
{
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	GetSwapchain()->AcquireNextImage(GetPresentCompleteSemaphore());
	m_pDebugUI->NewFrame();
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &GetPresentCompleteSemaphore();
	VkPipelineStageFlags waitDstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitDstMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &GetRenderCompleteSemaphore();
	submitInfo.commandBufferCount = 1;
	if (m_UseRaymarching)
	{
		submitInfo.pCommandBuffers = &m_RaymarchDrawCommandBuffers[GetSwapchain()->GetActiveImageId()];
	}else
	if (m_UseInstancing)
	{
		submitInfo.pCommandBuffers = &m_InstancedDrawCommandBuffers[GetSwapchain()->GetActiveImageId()];
	}else
	{
		submitInfo.pCommandBuffers = &m_NoInstanceDrawCommandBuffers[GetSwapchain()->GetActiveImageId()];
	}

	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	
	m_pDebugUI->Render(GetRenderCompleteSemaphore(), GetFrameBuffers()[GetSwapchain()->GetActiveImageId()], {m_pDebugWindow, m_pDebugStatWindow});

	GetSwapchain()->PresentImage(m_pDebugUI->GetDebugRenderCompleteSemaphore());
	std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
	m_RenderTime = std::chrono::duration<float>(t2 - t1).count()*1000;
	t1 = t2;

	m_Framerate = m_RenderTime + m_UpdateTime;
	m_FPS = 1 / (m_Framerate/1000);
}

bool VulkanApp::Update(float dTime)
{
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	m_ElapsedSec += dTime;
	glm::vec2 mousePos = GetWindow()->GetMousePos();
	glm::vec2 mouseMovement = mousePos - m_PrevMousePos;
	m_PrevMousePos = mousePos;
	if(GetWindow()->IsKeyPressed(VK_LSHIFT))
	{
		m_Camera.SetMovementSpeed(100.f);
	}
	else
	{
		m_Camera.SetMovementSpeed(5.f);
	}
	Camera_Movement movementDir{Camera_Movement::NONE};
	if(GetWindow()->IsKeyPressed('W')) movementDir = Camera_Movement::FORWARD;
	if(GetWindow()->IsKeyPressed('S')) movementDir = Camera_Movement::BACKWARD;
	if(GetWindow()->IsKeyPressed('A')) movementDir = Camera_Movement::LEFT;
	if(GetWindow()->IsKeyPressed('D')) movementDir = Camera_Movement::RIGHT;
	m_Camera.SetMovementSpeed(m_CameraSpeed);
	m_Camera.ProcessKeyboard(movementDir, dTime);
	float sensitivity = 3.f;
	
	if(GetWindow()->IsMouseButtonPressed(MouseButton::LEFT) && m_ShouldCaptureMouse)
	{
		m_Camera.ProcessMouseMovement(sensitivity * mouseMovement.x, sensitivity * mouseMovement.y, true);
	}
	UpdateUniformBuffers(dTime);
	std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
	m_UpdateTime = std::chrono::duration<float>(t2 - t1).count()*1000;
	t1 = t2;
	return VulkanBaseApp::Update(dTime);
}

void VulkanApp::Init(uint32_t width, uint32_t height)
{
	//EnableRaytracingExtension();
	VulkanBaseApp::Init(width, height);
	m_pUniformBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size_t(sizeof(CameraInfo)), &m_Ubo);
	m_pRaymarchUniformBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size_t(sizeof(RaymarchInfo)), &m_RaymarchUbo);
	CreateInstancedVertexBuffer();
	CreateNoInstanceVertexBuffer();
	m_pDescriptorPool = new vkw::DescriptorPool(GetDevice());
	//m_pInstancedDescriptorSet = new vkw::DescriptorSet();
	//m_pInstancedDescriptorSet->AddBinding(m_pUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	//m_pInstancedDescriptorSet->AddBinding(m_pTerrainBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pNoInstanceDescriptorSet = new vkw::DescriptorSet();
	m_pNoInstanceDescriptorSet->AddBinding(m_pUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	//m_pRaymarchDescriptorSet = new vkw::DescriptorSet();
	//m_pRaymarchDescriptorSet->AddBinding(m_pRaymarchUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//m_pRaymarchDescriptorSet->AddBinding(m_pTerrainBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//m_pDescriptorPool->AddDescriptorSet(m_pInstancedDescriptorSet);
	m_pDescriptorPool->AddDescriptorSet(m_pNoInstanceDescriptorSet);
	//m_pDescriptorPool->AddDescriptorSet(m_pRaymarchDescriptorSet);
	m_pDescriptorPool->Allocate();
	//m_pInstancedGraphicsPipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pInstancedDescriptorSet->GetLayout(), &m_pInstancedVertexBuffer->GetLayout(), "../Shaders/InstancedPerspective.vert.spv", "../Shaders/Color.frag.spv", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE);
	m_pNoInstanceGraphicsPipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pNoInstanceDescriptorSet->GetLayout(), &m_pNoInstanceVertexBuffer->GetLayout(), "../Shaders/MeshDebugRendering/ColorNormalPerspective.vert.spv", "../Shaders/MeshDebugRendering/Diffuse.frag.spv", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE);
	//m_pRaymarchGraphicsPipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pRaymarchDescriptorSet->GetLayout(), nullptr, "../Shaders/Raymarched.vert.spv", "../Shaders/Raymarched.frag.spv", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	m_pDebugUI = new vkw::DebugUI(GetDevice(), GetCommandPool(), GetWindow(), GetSwapchain(), GetDepthStencilBuffer());
	m_pDebugWindow = new vkw::DebugWindow{ "Properties" };
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_CameraSpeed), "Camera");
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_ShouldCaptureMouse), "Camera");
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_UseInstancing));
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_UseRaymarching));
	m_pDebugWindow->AddUIElement(new vkw::ShaderEditor("../Shaders/MeshDebugRendering/ColorNormalPerspective.vert"), "Shader");
	InitDebugStatWindow();
	BuildDrawCommandBuffers();
	m_PrevMousePos = GetWindow()->GetMousePos();
}

void VulkanApp::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	delete m_pDebugWindow;
	delete m_pDebugUI;
	//delete m_pInstancedGraphicsPipeline;
	delete m_pNoInstanceGraphicsPipeline;
	//delete m_pRaymarchGraphicsPipeline;
	delete m_pDescriptorPool;
	delete m_pUniformBuffer;
	delete m_pRaymarchUniformBuffer;
	delete m_pTerrainBuffer;
	delete m_pInstancedVertexBuffer;
	delete m_pNoInstanceVertexBuffer;
	delete m_pIndexBuffer;
	VulkanBaseApp::Cleanup();
}

void VulkanApp::EnableRaytracingExtension()
{
	GetDevice()->EnableInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);
}

void VulkanApp::CreateInstancedVertexBuffer()
{
	glm::vec3 cubeVertices[] = {
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

	bool* terrain = new bool[1000];

	for (size_t i = 0; i < 1000; i++)
	{
		terrain[i] = false;
	}

	for (size_t y = 0; y < 10; y++)
	{
		for (size_t z = 0; z < 10; z++)
		{
			for (size_t x = 0; x < 10; x++)
			{
				terrain[y * 100 + z * 10 + x] = true;
			}
		}
	}

	m_pTerrainBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1000, terrain);

	m_InstancedVertexCount = sizeof(cubeVertices)/sizeof(glm::vec3);
	m_pInstancedVertexBuffer = new vkw::VertexBuffer(GetDevice(), GetCommandPool(), vkw::VertexLayout({ VertexAttribute::POSITION  }), sizeof(cubeVertices), cubeVertices);
}

void VulkanApp::CreateNoInstanceVertexBuffer()
{
	Mesh* cubeMesh = CreateCubeMesh(1.f, {0.f,0.f}, {0.3f, 0.6f, 0.f, 1.f});
	std::vector<VertexAttribute> attributes = { VertexAttribute::POSITION, VertexAttribute::COLOR, VertexAttribute::NORMAL };
	size_t cubeDataSize = cubeMesh->GetVertexDataSize(attributes);

	const size_t size{ 16 };
	Array3D<uint32_t> map{ size,size,size };
	for (size_t i = 0; i < size; i++)
	{
		map[size / 2][i][size / 2] = 1;
	}
	std::vector<float> vertices{};
	vertices.resize(size*size*size*cubeDataSize);
	float* writePos = vertices.data();
	size_t vertexOffset = 0;
	std::vector<uint32_t> indices{};
	indices.resize(cubeMesh->GetIndexCount()* size*size*size);
	uint32_t* indexWritePos = indices.data();
	for (size_t y = 0; y < size; y++)
	{
		for (size_t z = 0; z < size; z++)
		{
			for (size_t x = 0; x < size; x++)
			{
				if (map[x][y][z] == 0)
				{
					glm::mat4x4 translation = glm::translate(glm::mat4x4(1.f), { x, y , z });
					writePos = cubeMesh->CreateVertices(attributes, writePos, translation);
					indexWritePos = cubeMesh->GetIndices(vertexOffset, indexWritePos);
					vertexOffset += cubeMesh->GetVertexCount(attributes);
				}
			}
		}
	}

	m_pIndexBuffer = new vkw::IndexBuffer(GetDevice(), GetCommandPool(), indices.size(), indices.data());
	m_pNoInstanceVertexBuffer = new vkw::VertexBuffer(GetDevice(), GetCommandPool(), vkw::VertexLayout(attributes), vertices.size(), vertices.data());

}

void VulkanApp::AllocateDrawCommandBuffers()
{
	//m_InstancedDrawCommandBuffers.resize(GetSwapchain()->GetImageCount());
	m_NoInstanceDrawCommandBuffers.resize(GetSwapchain()->GetImageCount());
	m_RaymarchDrawCommandBuffers.resize(GetSwapchain()->GetImageCount());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = GetCommandPool()->GetHandle();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = uint32_t(m_NoInstanceDrawCommandBuffers.size());

	//ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_InstancedDrawCommandBuffers.data()));
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_NoInstanceDrawCommandBuffers.data()));
	//ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_RaymarchDrawCommandBuffers.data()));
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

	VkViewport viewport{};
	viewport.width = float(GetWindow()->GetSurfaceSize().width);
	viewport.height = -float(GetWindow()->GetSurfaceSize().height); //flip vulkan viewport so y is up
	viewport.x = 0.f;
	viewport.y = float(GetWindow()->GetSurfaceSize().height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.extent = GetWindow()->GetSurfaceSize();
	scissor.offset = { 0, 0 };
	
	//for (int32_t i = 0; i < m_InstancedDrawCommandBuffers.size(); ++i)
	//{
	//	// Set target frame buffer
	//	renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();
	//
	//	ErrorCheck(vkBeginCommandBuffer(m_InstancedDrawCommandBuffers[i], &cmdBufferBeginInfo));
	//
	//	vkCmdBeginRenderPass(m_InstancedDrawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	//
	//
	//	vkCmdSetViewport(m_InstancedDrawCommandBuffers[i], 0, 1, &viewport);
	//
	//	vkCmdSetScissor(m_InstancedDrawCommandBuffers[i], 0, 1, &scissor);
	//
	//	vkCmdBindDescriptorSets(m_InstancedDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInstancedGraphicsPipeline->GetLayout(), 0, 1, &m_pInstancedDescriptorSet->GetHandle(), 0, NULL);
	//
	//	vkCmdBindPipeline(m_InstancedDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInstancedGraphicsPipeline->GetPipeline());
	//	VkDeviceSize offsets[1] = { 0 };
	//	vkCmdBindVertexBuffers(m_InstancedDrawCommandBuffers[i], 0, 1, &m_pInstancedVertexBuffer->GetBuffer().GetHandle(), offsets);
	//	vkCmdDraw(m_InstancedDrawCommandBuffers[i], m_InstancedVertexCount, 1000000, 0, 0);
	//	//vkCmdBindIndexBuffer(GetDrawCommandBuffers()[i], m_pIndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	//	//vkCmdDrawIndexed(GetDrawCommandBuffers()[i], 6, 1, 0, 0, 1);
	//
	//
	//	vkCmdEndRenderPass(m_InstancedDrawCommandBuffers[i]);
	//
	//	ErrorCheck(vkEndCommandBuffer(m_InstancedDrawCommandBuffers[i]));
	//}


	for (int32_t i = 0; i < m_NoInstanceDrawCommandBuffers.size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_NoInstanceDrawCommandBuffers[i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_NoInstanceDrawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_NoInstanceDrawCommandBuffers[i], 0, 1, &viewport);

		vkCmdSetScissor(m_NoInstanceDrawCommandBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_NoInstanceDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pNoInstanceGraphicsPipeline->GetLayout(), 0, 1, &m_pNoInstanceDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_NoInstanceDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pNoInstanceGraphicsPipeline->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_NoInstanceDrawCommandBuffers[i], 0, 1, &m_pNoInstanceVertexBuffer->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_NoInstanceDrawCommandBuffers[i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_NoInstanceDrawCommandBuffers[i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);
		vkCmdEndRenderPass(m_NoInstanceDrawCommandBuffers[i]);
		ErrorCheck(vkEndCommandBuffer(m_NoInstanceDrawCommandBuffers[i]));
	}

	//for (int32_t i = 0; i < m_RaymarchDrawCommandBuffers.size(); ++i)
	//{
	//	// Set target frame buffer
	//	renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();
	//
	//	ErrorCheck(vkBeginCommandBuffer(m_RaymarchDrawCommandBuffers[i], &cmdBufferBeginInfo));
	//
	//	vkCmdBeginRenderPass(m_RaymarchDrawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	//
	//
	//	vkCmdSetViewport(m_RaymarchDrawCommandBuffers[i], 0, 1, &viewport);
	//
	//	vkCmdSetScissor(m_RaymarchDrawCommandBuffers[i], 0, 1, &scissor);
	//
	//	vkCmdBindDescriptorSets(m_RaymarchDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRaymarchGraphicsPipeline->GetLayout(), 0, 1, &m_pRaymarchDescriptorSet->GetHandle(), 0, NULL);
	//
	//	vkCmdBindPipeline(m_RaymarchDrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRaymarchGraphicsPipeline->GetPipeline());
	//	vkCmdDraw(m_RaymarchDrawCommandBuffers[i], 3, 1, 0, 0);
	//
	//	vkCmdEndRenderPass(m_RaymarchDrawCommandBuffers[i]);
	//	ErrorCheck(vkEndCommandBuffer(m_RaymarchDrawCommandBuffers[i]));
	//}

}

void VulkanApp::FreeDrawCommandBuffers()
{
	//vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_InstancedDrawCommandBuffers.size()), m_InstancedDrawCommandBuffers.data());
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_NoInstanceDrawCommandBuffers.size()), m_NoInstanceDrawCommandBuffers.data());
	//vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_RaymarchDrawCommandBuffers.size()), m_RaymarchDrawCommandBuffers.data());
}

void VulkanApp::UpdateUniformBuffers(float dTime)
{
	m_Ubo.projection = m_Camera.GetProjectionMatrix(float(GetWindow()->GetSurfaceSize().width), float(GetWindow()->GetSurfaceSize().height), 0.1f,  10000.f);
	m_Ubo.view = m_Camera.GetViewMatrix();
	m_Ubo.time += dTime;
	m_pUniformBuffer->Update(&m_Ubo, sizeof(m_Ubo), GetCommandPool());
	m_RaymarchUbo.position = glm::vec4(m_Camera.GetPosition(), 0.f);
	m_RaymarchUbo.forward = glm::vec4(m_Camera.GetFront(), 0.f);
	m_RaymarchUbo.right = m_Camera.GetRight();
	m_RaymarchUbo.up = m_Camera.GetUp();
	m_RaymarchUbo.aspectRatio = m_Camera.GetAspectRatio();
	m_RaymarchUbo.time += dTime;
	m_pRaymarchUniformBuffer->Update(&m_RaymarchUbo, sizeof(m_RaymarchUbo), GetCommandPool());
}

void VulkanApp::InitDebugStatWindow()
{
	m_pDebugStatWindow = new vkw::DebugWindow{ "Statistics" };
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_FPS));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_Framerate));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_RenderTime));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_UpdateTime));
}













