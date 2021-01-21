#include "App.h"
#include <DataHandling/MeshShapes.h>
#include <VulkanWrapper/GraphicsPipeline.h>
#include <VulkanWrapper/DescriptorPool.h>
#include <VulkanWrapper/DescriptorSet.h>
#include <VulkanWrapper/Buffer.h>
#include <VulkanWrapper/VertexBuffer.h>
#include <VulkanWrapper/Window.h>
#include <DebugUI/DebugUI.h>
#include <DebugUI/DebugWindow.h>
#include <DebugUI/DebugUIElements.h>
#include <VulkanWrapper/VulkanSwapchain.h>
#include <VulkanWrapper/CommandPool.h>
#include <VulkanWrapper/VulkanHelpers.h>
#include <VulkanWrapper/VulkanDevice.h>
#include <VulkanWrapper/FrameBuffer.h>
#include <VulkanWrapper/RenderPass.h>
#include <VulkanWrapper/IndexBuffer.h>

void App::Init(uint32_t width, uint32_t height)
{
	VulkanBaseApp::Init(width, height);
	m_pDebugUI = new vkw::DebugUI(GetDevice(), GetCommandPool(), GetWindow(), GetSwapchain(), GetDepthStencilBuffer());
	m_pDebugWindow = new vkw::DebugWindow("RenderModes");
	m_pRenderModeSelector = new vkw::SelectableList<std::vector<VkCommandBuffer>>("DrawCommandBuffer", &m_DrawCommandBuffers);
	m_pDebugWindow->AddUIElement(m_pRenderModeSelector);
	m_pPlaneMesh = CreateRectBox(1.f, 10.f, 3.f, { 0.f, 0.f }, {1.f, 0.f, 0.f, 1.f});
	VkExtent2D surfaceSize = GetWindow()->GetSurfaceSize();
	m_UniformBufferData.projection = m_Camera.GetProjectionMatrix(float(surfaceSize.width), float(surfaceSize.height), 0.001f, 10000.f);
	m_UniformBufferData.view = m_Camera.GetViewMatrix();
	m_pUniformBuffer = new vkw::Buffer(GetDevice(), GetCommandPool(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size_t(sizeof(CameraInfo)), &m_UniformBufferData);
	InitRenderModes();
	BuildDrawCommandBuffers();
}

void App::Render()
{
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

	submitInfo.pCommandBuffers = &m_pRenderModeSelector->GetSelectedItem()[GetSwapchain()->GetActiveImageId()];
	ErrorCheck(vkQueueSubmit(GetDevice()->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	m_pDebugUI->Render(GetRenderCompleteSemaphore(), GetFrameBuffers()[GetSwapchain()->GetActiveImageId()], { m_pDebugWindow });
	GetSwapchain()->PresentImage(m_pDebugUI->GetDebugRenderCompleteSemaphore());
}


bool App::Update(float dTime)
{
	glm::vec2 mousePos = GetWindow()->GetMousePos();
	glm::vec2 mouseMovement = mousePos - m_PrevMousePos;
	m_PrevMousePos = mousePos;
	if (GetWindow()->IsKeyPressed(VK_LSHIFT))
	{
		m_Camera.SetMovementSpeed(100.f);
	}
	else
	{
		m_Camera.SetMovementSpeed(5.f);
	}
	Camera_Movement movementDir{ Camera_Movement::NONE };
	if (GetWindow()->IsKeyPressed('W')) movementDir = Camera_Movement::FORWARD;
	if (GetWindow()->IsKeyPressed('S')) movementDir = Camera_Movement::BACKWARD;
	if (GetWindow()->IsKeyPressed('A')) movementDir = Camera_Movement::LEFT;
	if (GetWindow()->IsKeyPressed('D')) movementDir = Camera_Movement::RIGHT;
	m_Camera.SetMovementSpeed(m_CameraSpeed);
	m_Camera.ProcessKeyboard(movementDir, dTime);
	float sensitivity = 3.f;

	if (GetWindow()->IsMouseButtonPressed(MouseButton::LEFT) && m_ShouldCaptureMouse)
	{
		m_Camera.ProcessMouseMovement(sensitivity * mouseMovement.x, sensitivity * mouseMovement.y, true);
	}

	m_UniformBufferData.view = m_Camera.GetViewMatrix();
	m_pUniformBuffer->Update(&m_UniformBufferData, sizeof(CameraInfo), GetCommandPool());
	return VulkanBaseApp::Update(dTime);
}


void App::Cleanup()
{
	ErrorCheck(vkQueueWaitIdle(GetDevice()->GetQueue()));
	delete m_pDescriptorPool;
	delete m_pDebugWindow;
	delete m_pDebugUI;
	delete m_pIndexBuffer;
	delete m_pUniformBuffer;
	delete m_pPlaneMesh;

	for (auto pair : m_pRenderPipelines)
	{
		delete pair.second;
	}
	for (auto pair : m_pVertexBuffers)
	{
		delete pair.second;
	}

	VulkanBaseApp::Cleanup();
}

void App::AllocateDrawCommandBuffers()
{
	m_DrawCommandBuffers["Wireframe"].resize(GetSwapchain()->GetImageCount());
	m_DrawCommandBuffers["Color"].resize(GetSwapchain()->GetImageCount());
	m_DrawCommandBuffers["UV"].resize(GetSwapchain()->GetImageCount());
	m_DrawCommandBuffers["Normal"].resize(GetSwapchain()->GetImageCount());
	m_DrawCommandBuffers["Diffuse"].resize(GetSwapchain()->GetImageCount());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = GetCommandPool()->GetHandle();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = uint32_t(GetSwapchain()->GetImageCount());

	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers["Wireframe"].data()));
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers["Color"].data()));
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers["UV"].data()));
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers["Normal"].data()));
	ErrorCheck(vkAllocateCommandBuffers(GetDevice()->GetDevice(), &commandBufferAllocateInfo, m_DrawCommandBuffers["Diffuse"].data()));
}

void App::BuildDrawCommandBuffers()
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

	for (int32_t i = 0; i < m_DrawCommandBuffers["Wireframe"].size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers["Wireframe"][i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers["Wireframe"][i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers["Wireframe"][i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers["Wireframe"][i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers["Wireframe"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Wireframe"]->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_DrawCommandBuffers["Wireframe"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Wireframe"]->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_DrawCommandBuffers["Wireframe"][i], 0, 1, &m_pVertexBuffers["Color"]->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_DrawCommandBuffers["Wireframe"][i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_DrawCommandBuffers["Wireframe"][i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);


		vkCmdEndRenderPass(m_DrawCommandBuffers["Wireframe"][i]);

		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers["Wireframe"][i]));
	}

	for (int32_t i = 0; i < m_DrawCommandBuffers["Color"].size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers["Color"][i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers["Color"][i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers["Color"][i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers["Color"][i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers["Color"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Color"]->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_DrawCommandBuffers["Color"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Color"]->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_DrawCommandBuffers["Color"][i], 0, 1, &m_pVertexBuffers["Color"]->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_DrawCommandBuffers["Color"][i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_DrawCommandBuffers["Color"][i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);


		vkCmdEndRenderPass(m_DrawCommandBuffers["Color"][i]);

		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers["Color"][i]));
	}

	for (int32_t i = 0; i < m_DrawCommandBuffers["UV"].size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers["UV"][i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers["UV"][i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers["UV"][i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers["UV"][i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers["UV"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["UV"]->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_DrawCommandBuffers["UV"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["UV"]->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_DrawCommandBuffers["UV"][i], 0, 1, &m_pVertexBuffers["UV"]->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_DrawCommandBuffers["UV"][i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_DrawCommandBuffers["UV"][i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);


		vkCmdEndRenderPass(m_DrawCommandBuffers["UV"][i]);

		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers["UV"][i]));
	}

	for (int32_t i = 0; i < m_DrawCommandBuffers["Normal"].size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers["Normal"][i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers["Normal"][i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers["Normal"][i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers["Normal"][i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers["Normal"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Normal"]->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_DrawCommandBuffers["Normal"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Normal"]->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_DrawCommandBuffers["Normal"][i], 0, 1, &m_pVertexBuffers["Normal"]->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_DrawCommandBuffers["Normal"][i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_DrawCommandBuffers["Normal"][i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);


		vkCmdEndRenderPass(m_DrawCommandBuffers["Normal"][i]);

		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers["Normal"][i]));
	}


	for (int32_t i = 0; i < m_DrawCommandBuffers["Diffuse"].size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = GetFrameBuffers()[i]->GetHandle();

		ErrorCheck(vkBeginCommandBuffer(m_DrawCommandBuffers["Diffuse"][i], &cmdBufferBeginInfo));

		vkCmdBeginRenderPass(m_DrawCommandBuffers["Diffuse"][i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdSetViewport(m_DrawCommandBuffers["Diffuse"][i], 0, 1, &viewport);

		vkCmdSetScissor(m_DrawCommandBuffers["Diffuse"][i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_DrawCommandBuffers["Diffuse"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Diffuse"]->GetLayout(), 0, 1, &m_pDescriptorSet->GetHandle(), 0, NULL);

		vkCmdBindPipeline(m_DrawCommandBuffers["Diffuse"][i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pRenderPipelines["Diffuse"]->GetPipeline());
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_DrawCommandBuffers["Diffuse"][i], 0, 1, &m_pVertexBuffers["Diffuse"]->GetBuffer().GetHandle(), offsets);
		vkCmdBindIndexBuffer(m_DrawCommandBuffers["Diffuse"][i], m_pIndexBuffer->GetBuffer().GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_DrawCommandBuffers["Diffuse"][i], uint32_t(m_pIndexBuffer->GetIndexCount()), 1, 0, 0, 1);


		vkCmdEndRenderPass(m_DrawCommandBuffers["Diffuse"][i]);

		ErrorCheck(vkEndCommandBuffer(m_DrawCommandBuffers["Diffuse"][i]));
	}
}

void App::FreeDrawCommandBuffers()
{
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers["Wireframe"].size()), m_DrawCommandBuffers["Wireframe"].data());
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers["Color"].size()), m_DrawCommandBuffers["Color"].data());
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers["UV"].size()), m_DrawCommandBuffers["UV"].data());
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers["Normal"].size()), m_DrawCommandBuffers["Normal"].data());
	vkFreeCommandBuffers(GetDevice()->GetDevice(), GetCommandPool()->GetHandle(), uint32_t(m_DrawCommandBuffers["Diffuse"].size()), m_DrawCommandBuffers["Diffuse"].data());
}

void App::InitRenderModes()
{
	//Setup the descriptor pool and set for used for all the render pipelines
	m_pDescriptorPool = new vkw::DescriptorPool(GetDevice());
	m_pDescriptorSet = new vkw::DescriptorSet();
	m_pDescriptorSet->AddBinding(m_pUniformBuffer->GetDescriptor(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pDescriptorPool->AddDescriptorSet(m_pDescriptorSet);
	m_pDescriptorPool->Allocate();

	m_VertexAttributes["Color"] = { VertexAttribute::POSITION, VertexAttribute::COLOR };
	m_VertexAttributes["UV"] = { VertexAttribute::POSITION, VertexAttribute::UV };
	m_VertexAttributes["Normal"] = { VertexAttribute::POSITION, VertexAttribute::NORMAL };
	m_VertexAttributes["Diffuse"] = { VertexAttribute::POSITION, VertexAttribute::COLOR, VertexAttribute::NORMAL };

	m_pIndexBuffer = new vkw::IndexBuffer(GetDevice(), GetCommandPool(), m_pPlaneMesh->GetIndices().size(), m_pPlaneMesh->GetIndices().data());
	glm::mat4x4 transMatrix = glm::translate(glm::mat4x4(1.f), { 0, 0, 0 });
	glm::mat4x4 rotMatrix = glm::rotate(glm::mat4x4(1.f), -glm::pi<float>() / 4.f, { 0, 1, 0 });
	m_pVertexBuffers["Color"] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(),
														m_VertexAttributes["Color"], 
														m_pPlaneMesh->GetVertexDataSize(m_VertexAttributes["Color"]), 
														m_pPlaneMesh->CreateVertices(m_VertexAttributes["Color"], transMatrix*rotMatrix).data()
													 );

	m_pVertexBuffers["UV"] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(),
													m_VertexAttributes["UV"],
													m_pPlaneMesh->GetVertexDataSize(m_VertexAttributes["UV"]),
													m_pPlaneMesh->CreateVertices(m_VertexAttributes["UV"], transMatrix * rotMatrix).data()
												  );

	m_pVertexBuffers["Normal"] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(),
														m_VertexAttributes["Normal"],
														m_pPlaneMesh->GetVertexDataSize(m_VertexAttributes["Normal"]),
														m_pPlaneMesh->CreateVertices(m_VertexAttributes["Normal"], transMatrix * rotMatrix).data()
													  );

	m_pVertexBuffers["Diffuse"] = new vkw::VertexBuffer(GetDevice(), GetCommandPool(),
														m_VertexAttributes["Diffuse"],
														m_pPlaneMesh->GetVertexDataSize(m_VertexAttributes["Diffuse"]),
														m_pPlaneMesh->CreateVertices(m_VertexAttributes["Diffuse"], transMatrix * rotMatrix).data()
														);

	m_pRenderPipelines["Wireframe"] = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(),
															m_pDescriptorSet->GetLayout(), m_pVertexBuffers["Color"]->GetLayout(),
															"../Shaders/MeshDebugRendering/ColorPerspective.vert.spv", "../Shaders/MeshDebugRendering/Color.frag.spv",
															VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE, true
															);

	m_pRenderPipelines["Color"] = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(),
															m_pDescriptorSet->GetLayout(), m_pVertexBuffers["Color"]->GetLayout(),
															"../Shaders/MeshDebugRendering/ColorPerspective.vert.spv", "../Shaders/MeshDebugRendering/Color.frag.spv",
															VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE
														   );

	m_pRenderPipelines["UV"] = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(),
														 m_pDescriptorSet->GetLayout(), m_pVertexBuffers["UV"]->GetLayout(),
														 "../Shaders/MeshDebugRendering/UVPerspective.vert.spv", "../Shaders/MeshDebugRendering/UV.frag.spv",
														 VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE
														);

	m_pRenderPipelines["Normal"] = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(),
														 m_pDescriptorSet->GetLayout(), m_pVertexBuffers["Normal"]->GetLayout(),
														 "../Shaders/MeshDebugRendering/NormalPerspective.vert.spv", "../Shaders/MeshDebugRendering/Normal.frag.spv",
														 VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE
														);

	m_pRenderPipelines["Diffuse"] = new vkw::GraphicsPipeline(GetDevice(), GetRenderPass(), GetPipelineCache(),
															m_pDescriptorSet->GetLayout(), m_pVertexBuffers["Diffuse"]->GetLayout(),
															"../Shaders/MeshDebugRendering/ColorNormalPerspective.vert.spv", "../Shaders/MeshDebugRendering/Diffuse.frag.spv",
															VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FRONT_FACE_CLOCKWISE
															);
}
