#include "App.h"


void App::Render()
{
}


bool App::Update(float dTime)
{
	return VulkanBaseApp::Update(dTime);
}

void App::Init(uint32_t width, uint32_t height)
{
	VulkanBaseApp::Init(width, height);
}

void App::Cleanup()
{
	VulkanBaseApp::Cleanup();
}

void App::AllocateDrawCommandBuffers()
{

}

void App::BuildDrawCommandBuffers()
{
}

void App::FreeDrawCommandBuffers()
{
}
