#pragma once
#include "VulkanWrapper/VulkanBaseApp.h"
class App : vkw::VulkanBaseApp
{
public:
	App(vkw::VulkanDevice* pDevice):VulkanBaseApp(pDevice, "App"){};
	~App() {};
	void Init(uint32_t width, uint32_t height) override;
	bool Update(float dTime) override;
	void Render() override;
	void Cleanup() override;
protected:
	void AllocateDrawCommandBuffers() override;
	void BuildDrawCommandBuffers() override;
	void FreeDrawCommandBuffers() override;
};

