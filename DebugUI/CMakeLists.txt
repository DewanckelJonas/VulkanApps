file(GLOB DEBUGUI_SRC "*.cpp" "../Libs/imgui/*.cpp" "../Libs/imgui/misc/cpp/*.cpp")
file(GLOB DEBUGUI_HEADERS "*.h")

add_library(DebugUI STATIC ${DEBUGUI_HEADERS} ${DEBUGUI_SRC})
add_subdirectory(../Libs/glslang ../libs/glslang/build)
include_directories(../Libs/glslang)
target_link_libraries(DebugUI ${Base} ${VulkanWrapper} glslang SPIRV)