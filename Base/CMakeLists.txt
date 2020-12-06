file(GLOB BASE_SRC "*.cpp")
file(GLOB BASE_HEADERS "*.h")
add_library(Base STATIC ${BASE_HEADERS} ${BASE_SRC})
