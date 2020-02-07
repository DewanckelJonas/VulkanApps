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
