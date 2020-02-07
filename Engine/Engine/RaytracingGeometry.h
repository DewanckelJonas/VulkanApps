#pragma once
#include <vector>
#include "VulkanDevice.h"
#include "AccelerationStructure.h"
namespace vkw
{
	class Model;
	class CommandPool;
	class RaytracingGeometry
	{
	public:
		RaytracingGeometry(VulkanDevice* pDevice, CommandPool* pCommandPool, const std::vector<Model*>& scene, std::vector<GeometryInstance>& instances);
		~RaytracingGeometry();
		std::vector<AccelerationStructure*> GetBottomLevelAS();
		AccelerationStructure* GetTopLevelAS();

	private: 
		void Init(const std::vector<Model*>& scene,  std::vector<GeometryInstance>& instances);
		void CreateBottomLevelAccelerationStructure(const std::vector<Model*>& scene);
		void CreateTopLevelAccelerationStructure( std::vector<GeometryInstance>& instances);
		Buffer* CreateScratchMemory();

		void CleanupBottomLevelAccelerationStructure();
		void CleanupTopLevelAccelerationStructure();

		VulkanDevice*								m_pDevice = nullptr;
		CommandPool*								m_pCommandPool = nullptr;
		std::vector<AccelerationStructure*>			m_pBottomLevelAccelerations{};
		AccelerationStructure*						m_pTopLevelAcceleration{};
		std::vector<VkGeometryNV>					m_Geometries{};
	};

}

