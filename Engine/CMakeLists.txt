file(GLOB ENGINE_SRC "*.cpp" "../external/imgui/*.cpp")
file(GLOB ENGINE_HEADERS "*.h")
#IMGUI requires extra source files.
file(GLOB ADD_SOURCE "../Libs/imgui/*.cpp")
SET(SOURCE ${SOURCE} ${ADD_SOURCE})

add_library(Engine STATIC ${ENGINE_HEADERS} ${ENGINE_SRC})
target_link_libraries(Engine ${Vulkan_LIBRARY})