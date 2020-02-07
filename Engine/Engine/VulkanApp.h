#pragma once
#include "VulkanBaseApp.h"
#include <glm/glm.hpp>
#include <array>
#include "Camera.h"

namespace vkw {
	class Buffer;
	class Texture;
	class Model;
	class RaytracingGeometry;
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
	void GetRaytracingFunctionPointers();
	void CreateOffscreenRenderPass();
	void CreateOffscreenFrameBuffers();
	void CreatePipelines();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateDescriptorSetLayout();
	void BuildDrawCommandBuffers();
	void BuildOffscreenCommandBuffers();
	void CreateDisplayQuads();
	void CreateRaytracingScene();
	void CreateRaytracingDescriptorSet();
	void CreateRaytracingPipeline();
	void CreateShaderBindingTable();
	void CreateRaytracingCommandBuffer();
	void CreateRaytracingAttachment();

	void UpdateUniformBuffers();

	void DestroyPipelines();
	void DestroyDescriptorSetLayout();
	void DestroyDisplayQuads();
	void DestroyOffscreenFrameBuffers();
	void DestroyUniformBuffers();
	void DestroyDescriptorPool();
	void DestroyOffscreenCommandBuffer();
	void DestroyRaytracingScene();
	void DestroyRaytracingAttachment();
	void DestroyRaytracinDescriptorSet();
	void DestroyRaytracingPipeline();

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec4 instancePos[3];
	} m_OffscreenUniformBufferData, m_DeferredUniformBufferData;

	enum LightType
	{
		POINT_LIGHT = 0,
		SPHERICAL_LIGHT = 1,
		AREA_LIGHT = 2,
	};

	struct Light {
		glm::vec4 position;
		glm::vec3 color;
		float radiance;
		glm::vec3 normal;
		float radius;
		glm::vec2 extent;
		LightType lightType;
		float padding;
	};

	struct {
		Light lights[6];
		glm::vec4 viewPos;
		int currentFrame = 0;
	} m_DeferredLights;

	const int m_NumberOfPrevFrames = 4;

	vkw::Buffer*			m_pDeferredBuffer = nullptr;
	vkw::Buffer*			m_pOffscreenBuffer = nullptr;
	vkw::Buffer*			m_pLightBuffer = nullptr;

	struct Offscreen
	{
		vkw::RenderPass* renderPass = nullptr;
		vkw::FrameBuffer* frameBuffer = nullptr;
		vkw::Texture* posAttachment = nullptr;
		vkw::Texture* normAttachment = nullptr;
		vkw::Texture* albedoAttachment = nullptr;
		vkw::DepthStencilBuffer* depthAttachment = nullptr;
		VkDescriptorSet modelDescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSet backgroundDescriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;
	} m_Offscreen;

	vkw::Model*					m_pFloorModel = nullptr;
	vkw::Model*					m_pArmorModel = nullptr;

	vkw::Texture*				m_pFloorNormal = nullptr;
	vkw::Texture*				m_pFloorAlbedo = nullptr;
	vkw::Texture*				m_pArmorNormal = nullptr;
	vkw::Texture*				m_pArmorAlbedo = nullptr;

	vkw::Buffer*				m_pDisplayVertexBuffer = nullptr;
	vkw::Buffer*				m_pDisplayIndexBuffer = nullptr;
	uint32_t					m_DisplayIndexCount{};

	VkPipeline					m_DeferredPipeline = VK_NULL_HANDLE;
	VkPipelineLayout			m_DeferredPipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout		m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet				m_DescriptorSet = VK_NULL_HANDLE;

	VkDescriptorPool			m_DescriptorPool = VK_NULL_HANDLE;

	struct Raytracing 
	{
		vkw::RaytracingGeometry*							scene = nullptr;
		VkDescriptorPool									descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout								descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet										descriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout									pipelineLayout = VK_NULL_HANDLE;
		VkPipeline											pipeline = VK_NULL_HANDLE;
		VkPhysicalDeviceRayTracingPropertiesNV				deviceProperties{};
		vkw::Buffer*										shaderBindingTable{ nullptr };
		const size_t										nrOfGroups{ 3 };
		VkCommandBuffer										commandBuffer = VK_NULL_HANDLE;
		vkw::Texture*										attachment;
		VkSemaphore											semaphore = VK_NULL_HANDLE;
	};
	Raytracing m_Raytracing;

	struct RaytracingUniformBufferData
	{
		Light lights[6];
	} m_RaytracingLights;


	glm::vec2					m_PrevMousePos{};
	Camera						m_Camera{};
	float						m_ElapsedSec{};

	public:
		PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
		PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
		PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
};

