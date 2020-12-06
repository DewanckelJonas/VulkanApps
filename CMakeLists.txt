cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(NAME VulkanApps)

project(${NAME})

include_directories(Libs/glm)
include_directories(Libs/glfw/include)
include_directories(Libs/imgui)
include_directories(Libs/vulkan)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Libs/glfw)

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

file(GLOB SOURCE *.cpp )

# Build all apps
function(buildApps)
	foreach(APP ${APPS})
		buildApp(${APS})
	endforeach(APP)
endfunction(buildApps)


# Compiler specific stuff
IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

add_subdirectory(Base)
add_subdirectory(VulkanWrapper)
add_subdirectory(DataHandling)
add_subdirectory(DebugUI)
add_subdirectory(Apps)