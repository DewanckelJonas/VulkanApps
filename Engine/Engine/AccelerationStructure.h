#include <vector>
#include "VulkanDevice.h"
#include <vector>
#include <glm/glm.hpp>
namespace vkw
{
	struct GeometryInstance {
		glm::mat3x4 transform;
		uint32_t instanceId : 24;
		uint32_t mask : 8;
		uint32_t instanceOffset : 24;
		uint32_t flags : 8;
		uint64_t bottomLevelHandle;
	};

	class Buffer;
	class CommandPool;
	class AccelerationStructure
	{
	public:
		AccelerationStructure(VulkanDevice* pDevice, VkGeometryNV* geometries, const std::vector<GeometryInstance>& instances);
		~AccelerationStructure();

		const VkAccelerationStructureNV& GetStructure();
		void* GetHandle();
		VkMemoryRequirements2 GetMemoryRequirements();
		void Build(vkw::CommandPool* pCommandPool, vkw::Buffer* scratchBuffer);

	private:
		void Init();
		void GetFunctionAdresses();
		void Cleanup();
		VulkanDevice*					m_pDevice;

		VkAccelerationStructureNV		m_AccelerationStructure = VK_NULL_HANDLE;
		VkDeviceMemory					m_Memory = VK_NULL_HANDLE;
		VkGeometryNV*					m_pGeometry = nullptr;
		std::vector<GeometryInstance>	m_Instances{};
		void*							m_pAccelartionStructHandle = nullptr;

	public:
		PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
		PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
		PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
		PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
		PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
		PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
	};
}