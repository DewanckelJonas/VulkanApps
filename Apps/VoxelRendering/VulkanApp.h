#pragma once
#include "VulkanWrapper/VulkanBaseApp.h"
#include <glm/glm.hpp>
#include <array>
#include "Base/Camera.h"
#include "Apps/VoxelChunk.h"
#include <Base/Array3D.h>
#include <random>

namespace vkw {
	class Buffer;
	class RaytracingGeometry;
	class GraphicsPipeline;
	class ComputePipeline;
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
	void CreateTerrainVertexBuffer();
	void CreateParticleBuffer();
	void UpdateUniformBuffers(float dTime);
	void Reload();
	


	std::vector<VkCommandBuffer>	m_DrawCommandBuffers{};
	VkCommandBuffer					m_ComputeCommandBuffer = VK_NULL_HANDLE;


	vkw::GraphicsPipeline*			m_pNoInstanceGraphicsPipeline = nullptr;
	vkw::GraphicsPipeline*			m_pParticlePipeline = nullptr;
	vkw::ComputePipeline*			m_pComputePipeline = nullptr;

	vkw::DescriptorPool*			m_pDescriptorPool = nullptr;
	vkw::DescriptorSet*				m_pNoInstanceDescriptorSet = nullptr;

	vkw::Buffer*					m_pUniformBuffer = nullptr;
	std::vector<vkw::IndexBuffer*>	m_pIndexBuffers;
	std::vector<vkw::VertexBuffer*>	m_pVertexBuffers;
	vkw::Buffer*					m_pTerrainDataBuffer = nullptr;
	vkw::Buffer*					m_pParticleBuffer = nullptr;
	Array3D<VoxelChunk*>			m_pChunks;

	struct CameraInfo
	{
		glm::mat4x4 projection{};
		glm::mat4x4 view{};
		float time = 0.f;
	} m_Ubo;

	struct Particle 
	{
		glm::vec3 Position;
		float Size;
		glm::vec3 Velocity;
		float Alpha;
	};

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

