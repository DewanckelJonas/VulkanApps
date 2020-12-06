# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif()

IF (NOT Vulkan_FOUND)
	find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/Libs/vulkan" REQUIRED)
	IF (Vulkan_LIBRARY)
		set(Vulkan_FOUND ON)
		message("Using bundled Vulkan library version")
	ENDIF()
ENDIF()


IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

file(GLOB VULKANWRAPPER_SRC "*.cpp")
file(GLOB VULKANWRAPPER_HEADERS "*.h")

add_library(VulkanWrapper STATIC ${VULKANWRAPPER_HEADERS} ${VULKANWRAPPER_SRC})
target_link_libraries(VulkanWrapper ${Base} ${Vulkan_LIBRARY} ${XCB_LIBRARIES} glfw)