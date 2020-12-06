#include "AccelerationStructure.h"
#include "VulkanHelpers.h"
#include "CommandPool.h"
#include "Buffer.h"

using namespace vkw;

vkw::AccelerationStructure::AccelerationStructure(VulkanDevice* pDevice, VkGeometryNV* geometry, const std::vector<GeometryInstance>& instances)
	:m_pDevice{pDevice}, m_pGeometry{geometry}, m_Instances{instances}
{
	GetFunctionAdresses();
	Init();
}

vkw::AccelerationStructure::~AccelerationStructure()
{
	Cleanup();
}

const VkAccelerationStructureNV& vkw::AccelerationStructure::GetStructure()
{
	return m_AccelerationStructure;
}

void* vkw::AccelerationStructure::GetHandle()
{
	return m_pAccelartionStructHandle;
}

VkMemoryRequirements2 vkw::AccelerationStructure::GetMemoryRequirements()
{

	VkMemoryRequirements2 memRequirements{};
	memRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	memoryRequirementsInfo.accelerationStructure = m_AccelerationStructure;
	vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->GetDevice(), &memoryRequirementsInfo, &memRequirements);
	return memRequirements;
}

void vkw::AccelerationStructure::Build(vkw::CommandPool* pCommandPool, vkw::Buffer* scratchBuffer)
{

	VkAccelerationStructureInfoNV buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.pGeometries = m_pGeometry;
	if(m_pGeometry == nullptr)
	{
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.geometryCount = 0;
		buildInfo.instanceCount = uint32_t(m_Instances.size());
	}
	else
	{
		buildInfo.geometryCount = 1;
		buildInfo.instanceCount = 0;
	}
	
	VkCommandBuffer commandBuffer = pCommandPool->BeginSingleTimeCommands();
	Buffer* instanceBuffer{ nullptr };
	if (m_Instances.size() == 0) {
		vkCmdBuildAccelerationStructureNV(commandBuffer, &buildInfo, VK_NULL_HANDLE, 0, VK_FALSE, m_AccelerationStructure,
			VK_NULL_HANDLE, scratchBuffer->GetHandle(), 0);

		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	} else {

		instanceBuffer = new Buffer(m_pDevice, pCommandPool, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Instances.size()*sizeof(GeometryInstance), m_Instances.data());

		vkCmdBuildAccelerationStructureNV(commandBuffer, &buildInfo, instanceBuffer->GetHandle(), 0, VK_FALSE, m_AccelerationStructure,
			VK_NULL_HANDLE, scratchBuffer->GetHandle(), 0);
	}

	pCommandPool->EndSingleTimeCommands(commandBuffer);
	
	if(instanceBuffer != nullptr)
		delete instanceBuffer;
}

void vkw::AccelerationStructure::Init()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = m_pGeometry;
	if(m_pGeometry == nullptr)
	{
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		accelerationStructureInfo.instanceCount = uint32_t(m_Instances.size());
		accelerationStructureInfo.geometryCount = 0;
	}

	VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	ErrorCheck(vkCreateAccelerationStructureNV(m_pDevice->GetDevice(), &accelerationStructureCI, nullptr, &m_AccelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.accelerationStructure = m_AccelerationStructure;

	VkMemoryRequirements2 memoryRequirements2{};
	vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->GetDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(&m_pDevice->GetPhysicalDeviceMemoryProperties(), &memoryRequirements2.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	ErrorCheck(vkAllocateMemory(m_pDevice->GetDevice(), &memoryAllocateInfo, nullptr, &m_Memory));

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_AccelerationStructure;
	accelerationStructureMemoryInfo.memory = m_Memory;
	ErrorCheck(vkBindAccelerationStructureMemoryNV(m_pDevice->GetDevice(), 1, &accelerationStructureMemoryInfo));

	ErrorCheck(vkGetAccelerationStructureHandleNV(m_pDevice->GetDevice(), m_AccelerationStructure, sizeof(uint64_t), &m_pAccelartionStructHandle));
}

void vkw::AccelerationStructure::GetFunctionAdresses()
{
	vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkCreateAccelerationStructureNV"));
	vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkDestroyAccelerationStructureNV"));
	vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkBindAccelerationStructureMemoryNV"));
	vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkGetAccelerationStructureHandleNV"));
	vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkGetAccelerationStructureMemoryRequirementsNV"));
	vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(m_pDevice->GetDevice(), "vkCmdBuildAccelerationStructureNV"));
}

void vkw::AccelerationStructure::Cleanup()
{
	vkDestroyAccelerationStructureNV(m_pDevice->GetDevice(), m_AccelerationStructure, nullptr);
	vkFreeMemory(m_pDevice->GetDevice(), m_Memory, nullptr);
}

