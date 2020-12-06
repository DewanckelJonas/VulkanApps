#include "VulkanWrapper/VulkanDevice.h"
#include <chrono>
#include <iostream>
#include "App.h"

int main()
{
	vkw::VulkanDevice device{};
	App app{ &device };
	app.Init(1920, 1080);
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	bool isRunning{ true };
	while (isRunning)
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		float dTime = std::chrono::duration<float>(t2 - t1).count();
		t1 = t2;
		app.Render();
		isRunning = app.Update(dTime);
	}
	app.Cleanup();
	return 0;
}