
#include "VulkanWrapper/VulkanDevice.h"
#include <chrono>
#include <iostream>
#include <Base/Array3D.h>
#include "VulkanApp.h"

int main()
{

	Array3D<int> testArray{ 10, 20, 15 };

	for (size_t x = 0; x < 10; x++)
	{
		for (size_t y = 0; y < 20; y++)
		{
			for (size_t z = 0; z < 15; z++)
			{
				testArray[x][y][z] = 0;
			}
		}
	}
	testArray[0][5][4] = 5;
	for (size_t x = 0; x < 10; x++)
	{
		for(size_t y = 0; y < 20; y++)
		{
			for (size_t z = 0; z < 15; z++)
			{
				std::cout << testArray[x][y][z] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	vkw::VulkanDevice device{};
	VulkanApp app{ &device };
	app.Init(1920, 1080);
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	bool isRunning{ true };
	while(isRunning)
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