#pragma once
#include "Platform.h"
namespace vkw
{
	static const VkApplicationInfo APP_INFO
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO, //sType
		nullptr,							//pNext
		"Vulkan Framework",					//Application Name
		 VK_MAKE_VERSION(0, 1, 0),			//Application Version
		 nullptr,							//Engine Name
		 0,									//Engine Version
		 VK_API_VERSION_1_1					//API VERSION
	};
}

