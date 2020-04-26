# Function for building single app
function(buildApp APP_NAME)
	SET(APP_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME})
	message(STATUS "Generating project file for app in ${APP_FOLDER}")
	# Main
	file(GLOB SOURCE *.cpp ${ENGINE_HEADERS} ${APP_FOLDER}/*.cpp)
	file(GLOB HEADERS *.h ${APP_FOLDER}/*.h)
	SET(MAIN_CPP ${APP_FOLDER}/${APP_NAME}.cpp)
	if(EXISTS ${APP_FOLDER}/main.cpp)
		SET(MAIN_CPP ${APP_FOLDER}/main.cpp)
	ENDIF()
	
	# Add shaders
	set(SHADER_DIR "../Shaders/${APP_NAME}")
	file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp" "${SHADER_DIR}/*.geom" "${SHADER_DIR}/*.tesc" "${SHADER_DIR}/*.tese" "${SHADER_DIR}/*.mesh" "${SHADER_DIR}/*.task" "${SHADER_DIR}/*.rgen" "${SHADER_DIR}/*.rchit" "${SHADER_DIR}/*.rmiss")
	source_group("Shaders" FILES ${SHADERS})
	add_executable(${APP_NAME} ${HEADERS} ${MAIN_CPP} ${SOURCE} ${SHADERS})
	target_link_libraries(${APP_NAME} Engine)

	set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

endfunction(buildApp)

# Build all apps
function(buildApps)
	foreach(APP ${APPS})
		buildApp(${APPS})
	endforeach(APPS)
endfunction(buildApps)

set(APPS
	VoxelRendering
)

buildApps()