function(buildSource SAMPLE_NAME)
	file(GLOB SAMPLE_SOURCES 
		${SAMPLE_NAME}/*.cpp 
		${SAMPLE_NAME}/*.h 
		${SAMPLE_NAME}/*.inl)
	file(GLOB UTILITY_SOURCES 
		${SAMPLE_NAME}/Utilities/*.cpp 
		${SAMPLE_NAME}/Utilities/*.h 
		${SAMPLE_NAME}/Utilities/*.inl)
	file(GLOB VULKAN_SETUP_SOURCES 
		${SAMPLE_NAME}/VulkanSetup/*.cpp 
		${SAMPLE_NAME}/VulkanSetup/*.h 
		${SAMPLE_NAME}/VulkanSetup/*.inl)
	file(GLOB SCENE_ELEMENTS_SOURCES 
		${SAMPLE_NAME}/SceneElements/*.cpp 
		${SAMPLE_NAME}/SceneElements/*.h 
		${SAMPLE_NAME}/SceneElements/*.inl)
	file(GLOB_RECURSE SHADER_SOURCES
		${SAMPLE_NAME}/*.vert
		${SAMPLE_NAME}/*.frag
		${SAMPLE_NAME}/*.geom
		${SAMPLE_NAME}/*.comp
		${SAMPLE_NAME}/*.tese
		${SAMPLE_NAME}/*.tesc )
	
	SET(MAIN_CPP ${SAMPLE_NAME}/main.cpp)
	source_group("Shaders" FILES ${SHADER_SOURCES})
	source_group("Utilities" FILES ${UTILITY_SOURCES})
	source_group("VulkanSetup" FILES ${VULKAN_SETUP_SOURCES})
	source_group("SceneElements" FILES ${SCENE_ELEMENTS_SOURCES})

	if(WIN32)
		add_executable(${SAMPLE_NAME} ${MAIN_CPP} 
			${SAMPLE_SOURCES} 
			${UTILITY_SOURCES} 
			${SHADER_SOURCES} 
			${VULKAN_SETUP_SOURCES} 
			${SCENE_ELEMENTS_SOURCES})
		target_link_libraries(${SAMPLE_NAME} ${WINLIBS})
	else(WIN32)
		add_executable(${SAMPLE_NAME} ${MAIN_CPP} 
			${SAMPLE_SOURCES} 
			${UTILITY_SOURCES}
			${SHADER_SOURCES} 
			${VULKAN_SETUP_SOURCES} 
			${SCENE_ELEMENTS_SOURCES})
		target_link_libraries(${SAMPLE_NAME} ${CMAKE_THREAD_LIBS_INIT})
	endif(WIN32)

	foreach(SHADER_SOURCE ${SHADER_SOURCES})
		set(SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_NAME}/shaders)
		if(WIN32)
			get_filename_component(fname ${SHADER_SOURCE} NAME)
			add_custom_target(${SAMPLE_NAME}_${fname}.spv
							COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIR} && 
							$ENV{VK_SDK_PATH}/Bin/glslangValidator.exe -V ${SHADER_SOURCE} -o ${SHADER_DIR}/${fname}.spv
							SOURCES ${SHADER_SOURCE} )
			ExternalTarget("Shaders" ${SAMPLE_NAME}_${fname}.spv)
			add_dependencies(${SAMPLE_NAME} ${SAMPLE_NAME}_${fname}.spv)
		endif(WIN32)
	endforeach()

	target_link_libraries(${SAMPLE_NAME} Vulkan::Vulkan glfw)
	target_include_directories( ${SAMPLE_NAME} PRIVATE
								${CMAKE_CURRENT_SOURCE_DIR} 
								${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}
								${GLM_INCLUDE_DIR} )
			
	ExternalTarget("src" ${SAMPLE_NAME})
endfunction(buildSource)

buildSource(GraphicsPlayground)