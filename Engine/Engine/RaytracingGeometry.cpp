#include "RaytracingGeometry.h"
#include "Buffer.h"
#include <algorithm>

using namespace vkw;

vkw::RaytracingGeometry::RaytracingGeometry(VulkanDevice* pDevice, CommandPool* pCommandPool, const std::vector<Model*>& scene, std::vector<GeometryInstance>& instances)
	:m_pDevice{pDevice}, m_pCommandPool{pCommandPool}
{
	Init(scene, instances);
}

vkw::RaytracingGeometry::~RaytracingGeometry()
{
	CleanupTopLevelAccelerationStructure();
	CleanupBottomLevelAccelerationStructure();
}

std::vector<AccelerationStructure*> vkw::RaytracingGeometry::GetBottomLevelAS()
{
	return m_pBottomLevelAccelerations;
}

AccelerationStructure* vkw::RaytracingGeometry::GetTopLevelAS()
{
	return m_pTopLevelAcceleration;
}

void vkw::RaytracingGeometry::Init(const std::vector<Model*>& scene, std::vector<GeometryInstance>& instances)
{	
	CreateBottomLevelAccelerationStructure(scene);
	CreateTopLevelAccelerationStructure(instances);

	Buffer* pScratchMemory = CreateScratchMemory();

	for (size_t i = 0; i < m_pBottomLevelAccelerations.size(); i++)
	{
		m_pBottomLevelAccelerations[i]->Build(m_pCommandPool, pScratchMemory);
	}
	m_pTopLevelAcceleration->Build(m_pCommandPool, pScratchMemory);

	delete pScratchMemory;
}

void vkw::RaytracingGeometry::CreateBottomLevelAccelerationStructure(const std::vector<Model*>& scene)
{
	//TODO rewrite this once scene is implemented again.
	scene;
	//m_Geometries.resize(scene.size());
	//m_pBottomLevelAccelerations.resize(scene.size());
	////Create Bottom Level Structure
	//for (size_t i = 0; i < m_Geometries.size(); i++)
	//{
	//	m_Geometries[i].sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	//	m_Geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	//	m_Geometries[i].geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	//	m_Geometries[i].geometry.triangles.vertexData = scene[i]->GetVertexBuffer()->GetHandle();
	//	m_Geometries[i].geometry.triangles.vertexOffset = 0;
	//	m_Geometries[i].geometry.triangles.vertexCount = scene[i]->GetVertexCount();
	//	m_Geometries[i].geometry.triangles.vertexStride = scene[i]->GetVertexLayout().GetStride();
	//	m_Geometries[i].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	//	m_Geometries[i].geometry.triangles.indexData = scene[i]->GetIndexBuffer()->GetHandle();
	//	m_Geometries[i].geometry.triangles.indexOffset = 0;
	//	m_Geometries[i].geometry.triangles.indexCount = scene[i]->GetIndexCount();
	//	m_Geometries[i].geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	//	m_Geometries[i].geometry.triangles.transformData = VK_NULL_HANDLE;
	//	m_Geometries[i].geometry.triangles.transformOffset = 0;
	//	m_Geometries[i].geometry.aabbs = {};
	//	m_Geometries[i].geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	//	m_Geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_NV;
	//
	//	m_pBottomLevelAccelerations[i] = new AccelerationStructure(m_pDevice, &m_Geometries[i], {});
	//}

}

void vkw::RaytracingGeometry::CreateTopLevelAccelerationStructure(std::vector<GeometryInstance>& instances)
{
	for (size_t i = 0; i < instances.size(); i++)
	{
		instances[i].bottomLevelHandle = uint64_t(m_pBottomLevelAccelerations[instances[i].bottomLevelHandle]->GetHandle());
	}
	m_pTopLevelAcceleration = new AccelerationStructure(m_pDevice, nullptr, instances);
}

vkw::Buffer* vkw::RaytracingGeometry::CreateScratchMemory()
{
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	VkDeviceSize scratchBufferSize = 0;
	for (size_t i = 0; i < m_pBottomLevelAccelerations.size(); i++)
	{
		VkMemoryRequirements2 memReqBottomLevelAS = m_pBottomLevelAccelerations[i]->GetMemoryRequirements();
		scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, scratchBufferSize);
	}

	VkMemoryRequirements2 memReqTopLevelAS = m_pTopLevelAcceleration->GetMemoryRequirements();
	scratchBufferSize = std::max(memReqTopLevelAS.memoryRequirements.size, scratchBufferSize);
	
	return new vkw::Buffer(m_pDevice, m_pCommandPool, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBufferSize, nullptr);
}

void vkw::RaytracingGeometry::CleanupBottomLevelAccelerationStructure()
{
	for (size_t i = 0; i < m_pBottomLevelAccelerations.size(); i++)
	{
		delete m_pBottomLevelAccelerations[i];
	}
}

void vkw::RaytracingGeometry::CleanupTopLevelAccelerationStructure()
{
	delete m_pTopLevelAcceleration;
}
