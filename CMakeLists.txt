### ${CMAKE_SOURCE_DIR} is defined by us in the cmake-gui as "D:/Documents/Github/ShaderPlayGround"
### ${CMAKE_BINARY_DIR} is defined by us in the cmake-gui as "D:/Documents/Github/ShaderPlayGround/build"

### This project is only being tested on a x64 build with Visual Studio 2017. Uses win32.

cmake_minimum_required(VERSION 3.5)
project(GraphicsPlayground)

##############################################
## set CMAKE variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

## Many old CRT functions have newer, more secure versions. If a secure function exists, the older,
## less secure version is marked as deprecated
## https://docs.microsoft.com/en-us/cpp/c-runtime-library/security-features-in-the-crt?view=vs-2017
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

##############################################

include_directories(external)
include_directories(external/glm)
include_directories(external/glfw)
include_directories(external/assimp)

##############################################

### Need Vulkan for this project
## Use FindVulkan module added with CMAKE 3.7
find_package(Vulkan REQUIRED)

### Find and use the appropriate threads package for this platform
find_package(Threads REQUIRED)

IF(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSE(WIN32)
	find_package(Threads REQUIRED)
	# XCB (X protocol C-language Binding) is a library implementing the client-side of the X11 display server protocol
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ENDIF(WIN32)

### Error Checking to make sure we have all the libraries we need; Right now only checking for vulkan
IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

##############################################

# Enable the creation of folders for Visual Studio projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(ExternalTarget folder target)
	set_property(TARGET ${target} PROPERTY FOLDER ${folder})
endfunction(ExternalTarget)

function(InternalTarget folder target)
	ExternalTarget("${folder}" ${target})
	if (MSVC)
	get_target_property(targetSources ${target} SOURCES)
		foreach(sourceFile IN ITEMS ${targetSources})
			if (IS_ABSOLUTE "${sourceFile}")
				file(RELATIVE_PATH sourceFile "${CMAKE_CURRENT_SOURCE_DIR}" "${sourceFile}")
			endif()
			get_filename_component(sourceDir "${sourceFile}" PATH)
			string(REPLACE "/" "\\" sourceDir "${sourceDir}")
			source_group("${sourceDir}" FILES "${sourceFile}")
		endforeach()
	endif()
endfunction(InternalTarget)

##############################################

# Compiler specific stuff
IF(MSVC)
	## Catches C++ exceptions only and tells the compiler to assume that functions declared as extern "C" never throw a C++ exception
	## https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2017
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

##############################################

IF(WIN32)
  # Nothing here (yet)
ELSE(WIN32)
	link_libraries(${XCB_LIBRARIES} ${VULKAN_LIB})
ENDIF(WIN32)

##############################################

add_subdirectory(external)
add_subdirectory(src)