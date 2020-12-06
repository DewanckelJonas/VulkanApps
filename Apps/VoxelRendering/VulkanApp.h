#pragma once
#include "VulkanWrapper/VulkanBaseApp.h"
#include <glm/glm.hpp>
#include <array>
#include "Base/Camera.h"


namespace vkw {
	class Buffer;
	class RaytracingGeometry;
	class GraphicsPipeline;
	class DescriptorPool;
	class DescriptorSet;
	class VertexBuffer;
	class DebugUI;
	class DebugWindow;
	class IndexBuffer;
}

class Mesh;

class VulkanApp : vkw::VulkanBaseApp
{
public:
	VulkanApp(vkw::VulkanDevice* pDevice);
	~VulkanApp();
	void Render() override;
	bool Update(float dTime) override;
	void Init(uint32_t width, uint32_t height) override;
	void Cleanup() override;
protected:
	void AllocateDrawCommandBuffers() override;
	void BuildDrawCommandBuffers() override;
	void FreeDrawCommandBuffers() override;
private:
	void EnableRaytracingExtension();
	void CreateInstancedVertexBuffer();
	void CreateNoInstanceVertexBuffer();
	void UpdateUniformBuffers(float dTime);
	

	vkw::Buffer*					m_pUniformBuffer = nullptr;
	vkw::Buffer*					m_pRaymarchUniformBuffer = nullptr;
	uint32_t						m_InstancedVertexCount = 0;
	uint32_t						m_NoInstanceVertexCount = 0;
	std::vector<VkCommandBuffer>	m_InstancedDrawCommandBuffers{};
	std::vector<VkCommandBuffer>	m_NoInstanceDrawCommandBuffers{};
	std::vector<VkCommandBuffer>	m_RaymarchDrawCommandBuffers{};
	vkw::VertexBuffer*				m_pInstancedVertexBuffer = nullptr;
	vkw::VertexBuffer*				m_pNoInstanceVertexBuffer = nullptr;
	vkw::Buffer*					m_pTerrainBuffer = nullptr;
	vkw::IndexBuffer*				m_pIndexBuffer = nullptr;

	vkw::GraphicsPipeline*			m_pInstancedGraphicsPipeline = nullptr;
	vkw::GraphicsPipeline*			m_pNoInstanceGraphicsPipeline = nullptr;
	vkw::GraphicsPipeline*			m_pRaymarchGraphicsPipeline = nullptr;

	vkw::DescriptorPool*			m_pDescriptorPool = nullptr;
	vkw::DescriptorSet*				m_pInstancedDescriptorSet = nullptr;
	vkw::DescriptorSet*				m_pNoInstanceDescriptorSet = nullptr;
	vkw::DescriptorSet*				m_pRaymarchDescriptorSet = nullptr;


	struct CameraInfo
	{
		glm::mat4x4 projection{};
		glm::mat4x4 view{};
		float time = 0.f;
	} m_Ubo;

	struct RaymarchInfo
	{
		glm::vec4 position;
		glm::vec4 forward;
		glm::vec3 right;
		float time = 0.f;
		glm::vec3 up;
		float aspectRatio;
	} m_RaymarchUbo;

	vkw::DebugUI*					m_pDebugUI = nullptr;
	vkw::DebugWindow*				m_pDebugWindow = nullptr;
	//Camera
	glm::vec2						m_PrevMousePos{};
	Camera							m_Camera{};
	float							m_ElapsedSec{};
	float							m_CameraSpeed = 20.f;
	bool							m_ShouldCaptureMouse = true;

	//Game
	bool							m_UseInstancing = false;
	bool							m_UseRaymarching = false;

	//Stats
	void InitDebugStatWindow();

	vkw::DebugWindow*				m_pDebugStatWindow = nullptr;
	float							m_RenderTime{};
	float							m_UpdateTime{};
	float							m_Framerate{};
	float							m_FPS{};


	public:
		PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
		PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
		PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
};

