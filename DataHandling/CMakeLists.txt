file(GLOB DATAHANDLING_SRC "*.cpp")
file(GLOB DATAHANDLING_HEADERS "*.h")

add_library(DataHandling STATIC ${DATAHANDLING_HEADERS} ${DATAHANDLING_SRC})
target_link_libraries(DataHandling Base)