
#include "VulkanDevice.h"
#include "VulkanBaseApp.h"
#include <chrono>
#include <iostream>
#include "VulkanApp.h"

int main()
{
	vkw::VulkanDevice device{};
	VulkanApp app{ &device };
	app.Init(1920, 1080);
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	int frames{};
	bool isRunning{ true };
	while(isRunning)
	{
		frames++;
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		float dTime = std::chrono::duration<float>(t2 - t1).count();
		t1 = t2;
		app.Render();
		isRunning = app.Update(dTime);
		std::cout << "FPS: " << 1 / dTime << std::endl;
	}
	app.Cleanup();
	return 0;
}