#pragma once
#include "VulkanBaseApp.h"
#include <glm/glm.hpp>
#include <array>
#include "Camera.h"

namespace vkw {
	class Buffer;
	class RaytracingGeometry;
	class GraphicsPipeline;
	class DescriptorPool;
	class DescriptorSet;
	class VertexBuffer;
}
class VulkanApp : vkw::VulkanBaseApp
{
public:
	VulkanApp(vkw::VulkanDevice* pDevice);
	~VulkanApp();
	void Render() override;
	bool Update(float dTime) override;
	void Init(uint32_t width, uint32_t height) override;
	void Cleanup() override;

private:
	void EnableRaytracingExtension();
	void CreateVertexBuffer();
	void BuildDrawCommandBuffers();
	void UpdateUniformBuffers();

	vkw::Buffer*				m_pUniformBuffer = nullptr;
	uint32_t					m_VertexCount = 0;
	vkw::VertexBuffer*			m_pVertexBuffer = nullptr;
	vkw::Buffer*				m_pIndexBuffer = nullptr;
	vkw::Buffer*				m_pTerrainBuffer = nullptr;

	vkw::GraphicsPipeline*		m_pGraphicsPipeline = nullptr;
	vkw::DescriptorPool*		m_pDescriptorPool = nullptr;
	vkw::DescriptorSet*			m_pDescriptorSet = nullptr;

	struct CameraInfo
	{
		glm::mat4x4 projection{};
		glm::mat4x4 view{};
	} m_Ubo;

	glm::vec2					m_PrevMousePos{};
	Camera						m_Camera{};
	float						m_ElapsedSec{};

	public:
		PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
		PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
		PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
};

