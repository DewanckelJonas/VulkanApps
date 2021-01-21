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
#include <DebugUI/Button.h>

const uint32_t ParticleCount = 100000;

VulkanApp::VulkanApp(vkw::VulkanDevice* pDevice)
	:VulkanBaseApp(pDevice, "VoxelTest")
	,m_pChunks{1, 1, 1}
	,m_pVertexBuffers{}
	,m_pIndexBuffers{}
{
	const size_t chunkSize = 16;
	int width = m_pChunks.GetWidth();
	int height = m_pChunks.GetHeight();
	int depth = m_pChunks.GetDepth();
	for (size_t x = 0; x < width; x++)
	{
		for (size_t y = 0; y < height; y++)
		{
			for (size_t z = 0; z < depth; z++)
			{
				m_pChunks[x][y][z] = new VoxelChunk{ {chunkSize, chunkSize, chunkSize}, glm::vec3{x * chunkSize, y * chunkSize, z * chunkSize} };
			}
		}
	}
	m_pIndexBuffers.resize(width*height*depth);
	m_pVertexBuffers.resize(width*height*depth);
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
	submitInfo.pCommandBuffers = &m_DrawCommandBuffers[GetSwapchain()->GetActiveImageId()];


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
	if(GetWindow()->IsMouseButtonPressed(MouseButton::RIGHT))
	{
		glm::ivec3 id;
		Ray ray{ m_Camera.GetPosition(), m_Camera.GetFront() };
		for (size_t i = 0; i < m_pIndexBuffers.size(); i++)
		{
			if(m_pChunks.Data()[i]->Raycast(id, ray, 0, 8.f))
			{
				const int brushSize = 1;
				for (int x = -brushSize; x < brushSize; x++)
				{
					for (int y = -brushSize; y < brushSize; y++)
					{
						for (int z = -brushSize; z < brushSize; z++)
						{
							glm::ivec3 chunkSize = { m_pChunks.Data()[i]->GetData().GetWidth(), m_pChunks.Data()[i]->GetData().GetHeight(), m_pChunks.Data()[i]->GetData().GetDepth() };
							glm::ivec3 voxelId{};
							voxelId.x = glm::min(glm::max(id.x + x, 0), chunkSize.x);
							voxelId.y = glm::min(glm::max(id.y + y, 0), chunkSize.y);
							voxelId.z = glm::min(glm::max(id.z + z, 0), chunkSize.z);
							m_pChunks.Data()[i]->GetData()[voxelId.x][voxelId.y][voxelId.z] = 0;
						}
					}
				}
				m_pChunks.Data()[i]->GenerateMesh();
				delete m_pIndexBuffers[i];
				delete m_pVertexBuffers[i];
				std::vector<VertexAttribute> attributes = { VertexAttribute::POSITION, VertexAttribute::COLOR, VertexAttribute::NORMAL };
				m_pIndexBuffers[i] = new vkw::IndexBuffer(GetDevice(), GetCommandPool(), m_pChunks.Data()[i]->GetIndexBuffer().size(), m_pChunks.Data()[i]->GetIndexBuffer().data());
				m_pVertexBuffers[i] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(), vkw::VertexLayout(attributes), m_pChunks.Data()[i]->GetVertexBuffer().size() * sizeof(float), m_pChunks.Data()[i]->GetVertexBuffer().data());
				Reload();
				break;
			}
		}
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
	CreateTerrainVertexBuffer();
	CreateParticleBuffer();
	m_pDescriptorPool = new vkw::DescriptorPool(GetDevice());
	m_pNoInstanceDescriptorSet = new vkw::DescriptorSet();
	m_pNoInstanceDescriptorSet->AddBinding(m_pUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pDescriptorPool->AddDescriptorSet(m_pNoInstanceDescriptorSet);
	m_pDescriptorPool->Allocate();
	m_pNoInstanceGraphicsPipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pNoInstanceDescriptorSet->GetLayout(), m_pVertexBuffers[0]->GetLayout(), "../Shaders/MeshDebugRendering/ColorNormalPerspective.vert.spv", "../Shaders/MeshDebugRendering/Diffuse.frag.spv");
	vkw::VertexLayout particleLayout{ {VertexAttribute::POSITION, VertexAttribute::FLOAT, VertexAttribute::VEC3, VertexAttribute::FLOAT} };
	m_pParticlePipeline = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(), m_pNoInstanceDescriptorSet->GetLayout(), particleLayout.GetLayout(), "../Shaders/Particles/Particle.vert.spv", "../Shaders/Particles/Particle.frag.spv", VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	m_pDebugUI = new vkw::DebugUI(GetDevice(), GetCommandPool(), GetWindow(), GetSwapchain(), GetDepthStencilBuffer());
	m_pDebugWindow = new vkw::DebugWindow{ "Properties" };
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_CameraSpeed), "Camera");
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_ShouldCaptureMouse), "Camera");
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_UseInstancing));
	m_pDebugWindow->AddUIElement(UI_CREATEPARAMETER(m_UseRaymarching));
	m_pDebugWindow->AddUIElement(new vkw::ShaderEditor("../Shaders/Particles/Particle.vert"), "Shader");
	std::function<void()> callBack = std::bind(&VulkanApp::Reload, this);
	m_pDebugWindow->AddUIElement(new vkw::Button("Rebuild Pipeline", callBack), "Shader");
	InitDebugStatWindow();
	BuildDrawCommandBuffers();
	m_PrevMousePos = GetWindow()->GetMousePos();
}

void VulkanApp::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	delete m_pDebugWindow;
	delete m_pDebugUI;
	delete m_pNoInstanceGraphicsPipeline;
	delete m_pParticlePipeline;
	delete m_pParticleBuffer;
	delete m_pDescriptorPool;
	delete m_pUniformBuffer;
	for (size_t i = 0; i < m_pVertexBuffers.size(); i++)
	{
		delete m_pVertexBuffers[i];
		delete m_pIndexBuffers[i];
		delete m_pChunks.Data()[i];
	}
	VulkanBaseApp::Cleanup();
}

void VulkanApp::EnableRaytracingExtension()
{
	GetDevice()->EnableInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	GetDevice()->EnableDeviceExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);
}

void VulkanApp::CreateTerrainVertexBuffer()
{
	std::vector<VertexAttribute> attributes = { VertexAttribute::POSITION, VertexAttribute::COLOR, VertexAttribute::NORMAL };
	for (size_t i = 0; i < m_pIndexBuffers.size(); i++)
	{
		const size_t chunkSize = m_pChunks.Data()[i]->GetData().GetWidth() * m_pChunks.Data()[i]->GetData().GetHeight() * m_pChunks.Data()[i]->GetData().GetDepth();
		for (size_t j = 0; j < chunkSize; j++)
		{
			m_pChunks.Data()[i]->GetData().Data()[j] = 1;
		}

		m_pChunks.Data()[i]->GenerateMesh();
		m_pIndexBuffers[i] = new vkw::IndexBuffer(GetDevice(), GetCommandPool(), m_pChunks.Data()[i]->GetIndexBuffer().size(), m_pChunks.Data()[i]->GetIndexBuffer().data());
		m_pVertexBuffers[i] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(), vkw::VertexLayout(attributes), m_pChunks.Data()[i]->GetVertexBuffer().size()*sizeof(float), m_pChunks.Data()[i]->GetVertexBuffer().data());
	}
	
}

void VulkanApp::CreateParticleBuffer()
{
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndPos(0.0f, 16.0f);
	std::uniform_real_distribution<float> rnd(-16.0f, 16.0f);
	std::uniform_real_distribution<float> rndAlpha(0.1f, 1.f);

	std::vector<Particle> particleBuffer(ParticleCount);
	for (auto& particle : particleBuffer) {
		particle.Position = glm::vec3(rnd(rndEngine), rnd(rndEngine), rnd(rndEngine));
		particle.Size = rndPos(rndEngine);
		particle.Velocity = glm::vec3(rnd(rndEngine), rnd(rndEngine), rnd(rndEngine));
		particle.Alpha = rndAlpha(rndEngine);
	}

	m_pParticleBuffer = new vkw::Buffer(GetDevice(), GetCommandPool()
		, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		,sizeof(Particle)*particleBuffer.size()
		,particleBuffer.data()
	);
}

void VulkanApp::AllocateDrawCommandBuffers()
{
	m_DrawCommandBuffers.resize(GetSwapchain()->GetImageCount());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = GetCommandPool()->GetHandle();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = uint32_t(m_DrawCommandBuffers.size());

	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers.data()));
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
	

	for (int32_t i = 0; i < m_DrawCommandBuffers.size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers[i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers[i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pNoInstanceGraphicsPipeline->GetLayout(), 0, 1, &m_pNoInstanceDescriptorSet->GetHandle(), 0, NULL);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindPipeline(m_DrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pNoInstanceGraphicsPipeline->GetPipeline());
		for (size_t j = 0; j < m_pIndexBuffers.size(); j++)
		{
			vkCmdBindVertexBuffers(m_DrawCommandBuffers[i], 0, 1, &m_pVertexBuffers[j]->GetBuffer().GetHandle(), offsets);
			vkCmdBindIndexBuffer(m_DrawCommandBuffers[i], m_pIndexBuffers[j]->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(m_DrawCommandBuffers[i], uint32_t(m_pIndexBuffers[j]->GetIndexCount()), 1, 0, 0, 1);
		}

		/*vkCmdBindDescriptorSets(m_DrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pParticlePipeline->GetLayout(), 0, 1, &m_pNoInstanceDescriptorSet->GetHandle(), 0, NULL);
		vkCmdBindPipeline(m_DrawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pParticlePipeline->GetPipeline());
		vkCmdBindVertexBuffers(m_DrawCommandBuffers[i], 0, 1, &m_pParticleBuffer->GetHandle(), offsets);
		vkCmdDraw(m_DrawCommandBuffers[i], ParticleCount, 1, 0, 0);*/
		vkCmdEndRenderPass(m_DrawCommandBuffers[i]);
		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers[i]));
	}
}

void VulkanApp::FreeDrawCommandBuffers()
{
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers.size()), m_DrawCommandBuffers.data());
}

void VulkanApp::UpdateUniformBuffers(float dTime)
{
	m_Ubo.projection = m_Camera.GetProjectionMatrix(float(GetWindow()->GetSurfaceSize().width), float(GetWindow()->GetSurfaceSize().height), 0.1f,  10000.f);
	m_Ubo.view = m_Camera.GetViewMatrix();
	m_Ubo.time += dTime;
	m_pUniformBuffer->Update(&m_Ubo, sizeof(m_Ubo), GetCommandPool());
}

void VulkanApp::Reload()
{
	vkQueueWaitIdle(GetDevice()->GetQueue());
	FreeDrawCommandBuffers();
	m_pNoInstanceGraphicsPipeline->Rebuild();
	m_pParticlePipeline->Rebuild();
	AllocateDrawCommandBuffers();
	BuildDrawCommandBuffers();
}

void VulkanApp::InitDebugStatWindow()
{
	m_pDebugStatWindow = new vkw::DebugWindow{ "Statistics" };
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_FPS));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_Framerate));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_RenderTime));
	m_pDebugStatWindow->AddUIElement(UI_CREATESTAT(m_UpdateTime));
}













