#pragma once
#include <VulkanWrapper/VulkanBaseApp.h>
#include <map>
#include <Base/VertexTypes.h>
#include <Base/Camera.h>

//Application to test Mesh generation/loading using debug render pipelines

class Mesh;
namespace vkw
{
	class GraphicsPipeline;
	class Buffer;
	class DescriptorPool;
	class DescriptorSet;
	class VertexBuffer;
	class IndexBuffer;
	class DebugUI;
	class DebugWindow;
	template<class T>
	class SelectableList;
}
class App : vkw::VulkanBaseApp
{
public:
	App(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "MeshDebugRendering"){};
	~App() {};
	void Init(uint32_t width, uint32_t height) override;
	bool Update(float dTime) override;
	void Render() override;
	void Cleanup() override;

protected:
	void AllocateDrawCommandBuffers() override;
	void BuildDrawCommandBuffers() override;
	void FreeDrawCommandBuffers() override;

private:
	//Initializes graphicspipelines, vertexbuffers, index buffer and descriptorset for all render modes
	void InitRenderModes();


	vkw::DebugUI*											m_pDebugUI = nullptr;
	vkw::DebugWindow*										m_pDebugWindow = nullptr;
	vkw::SelectableList<std::vector<VkCommandBuffer>>*		m_pRenderModeSelector = nullptr;

	std::map<std::string, vkw::GraphicsPipeline*>			m_pRenderPipelines{};
	std::map<std::string, std::vector<VkCommandBuffer>>		m_DrawCommandBuffers{};
	std::map<std::string, std::vector<VertexAttribute>>		m_VertexAttributes{};
	std::map<std::string, vkw::VertexBuffer*>				m_pVertexBuffers{}; 
	vkw::IndexBuffer*										m_pIndexBuffer{};

	vkw::Buffer*											m_pUniformBuffer = nullptr;
	vkw::DescriptorPool*									m_pDescriptorPool = nullptr;
	vkw::DescriptorSet*										m_pDescriptorSet = nullptr;

	Mesh*													m_pPlaneMesh = nullptr;

	//Camera stuff
	Camera							m_Camera{};
	glm::vec2						m_PrevMousePos{};
	float							m_ElapsedSec{};
	float							m_CameraSpeed = 20.f;
	bool							m_ShouldCaptureMouse = true;
	struct CameraInfo
	{
		glm::mat4x4 projection{};
		glm::mat4x4 view{};
	}m_UniformBufferData;
};

