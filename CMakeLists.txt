cmake_minimum_required(VERSION 3.3)

project(ShooterDemo)

# if we are compiling on windows, default to using local contrib include files and libraries
# otherwise, on linux, mac, search for global libraries since there might be version mismatch 
# between local include files, and system libraries
IF(WIN32)
	option(LocalIncludes "Use local includes" ON)
ELSE(WIN32)
	option(LocalIncludes "Use global includes" OFF)
ENDIF(WIN32)

# allow GCC to build with C++14
# need -fpermissive to forgive a few const errors
IF(CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "-Wall -fpermissive -std=c++14 -ggdb ${CMAKE_CXX_FLAGS}")
   set(FILESYSTEM_LIB stdc++fs)
ENDIF()

# extra CMAKE modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Setting up build structure

set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR})

SET(OUTPUT_BINDIR ${PROJECT_BINARY_DIR}/bin)
MAKE_DIRECTORY(${OUTPUT_BINDIR})

SET(OUTPUT_LIBDIR ${PROJECT_BINARY_DIR}/lib)
MAKE_DIRECTORY(${OUTPUT_LIBDIR})

SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${OUTPUT_LIBDIR} CACHE PATH "build directory")
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${OUTPUT_BINDIR} CACHE PATH "build directory")
IF(WIN32)
  SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${OUTPUT_BINDIR} CACHE PATH "build directory")
ELSE(WIN32)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR} CACHE PATH "build directory")
ENDIF(WIN32)

# For each configuration (Debug, Release, MinSizeRel... and/or anything the user chooses)
FOREACH(CONF ${CMAKE_CONFIGURATION_TYPES})
# Go uppercase (DEBUG, RELEASE...)
STRING(TOUPPER "${CONF}" CONF)
SET("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
IF(WIN32)
  SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
ELSE()
  SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
ENDIF()
ENDFOREACH()

# Windows build comes with included development Binaries and Libraries
IF(WIN32)
	set(WIN_LIBS_ARCHIVE_NAME "ShooterDemo_win7_x32_lib.tar.gz")
	set(WIN_LIBS_ARCHIVE_PATH ${PROJECT_BINARY_DIR}/lib/${WIN_LIBS_ARCHIVE_NAME})
	IF(NOT EXISTS ${WIN_LIBS_ARCHIVE_PATH})
		# Download Windows libraries
		message("Downloading Windows libraries ...")
		file(DOWNLOAD "https://www.dropbox.com/s/iv71iol32phzk14/${WIN_LIBS_ARCHIVE_NAME}?dl=1" ${WIN_LIBS_ARCHIVE_PATH} SHOW_PROGRESS)
	ENDIF()
		
	IF(NOT EXISTS ${PROJECT_BINARY_DIR}/lib/SDL2.lib)
		# Unpack libraries
		message("Unpacking Windows libraries ...")
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzvf ${WIN_LIBS_ARCHIVE_PATH}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	ENDIF()
	
	set(WIN_BINS_ARCHIVE_NAME "ShooterDemo_win7_x32_bin.tar.gz")
	set(WIN_BINS_ARCHIVE_PATH ${PROJECT_BINARY_DIR}/bin/${WIN_BINS_ARCHIVE_NAME})
	IF(NOT EXISTS ${WIN_BINS_ARCHIVE_PATH})
		# Download Windows binaries
		message("Downloading Windows binaries ...")
		file(DOWNLOAD "https://www.dropbox.com/s/ul2e4u2fcw7kjpl/${WIN_BINS_ARCHIVE_NAME}?dl=1" ${WIN_BINS_ARCHIVE_PATH} SHOW_PROGRESS)
	ENDIF()
	
	IF(NOT EXISTS ${PROJECT_BINARY_DIR}/bin/SDL2.dll)
		# Unpack binaries
		message("Unpacking Windows binaries ...")
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzvf ${WIN_BINS_ARCHIVE_PATH}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	ENDIF()
ENDIF(WIN32)

set(RES_ARCHIVE_NAME "ShooterDemo_res.tar.gz")
set(RES_ARCHIVE_PATH ${PROJECT_BINARY_DIR}/bin/${RES_ARCHIVE_NAME})
IF(NOT EXISTS ${RES_ARCHIVE_PATH})
	# Download resources
	message("Downloading resources ...")
	file(DOWNLOAD "https://www.dropbox.com/s/jt0g9rjg4u4bt2m/${RES_ARCHIVE_NAME}?dl=1" ${RES_ARCHIVE_PATH} SHOW_PROGRESS)
ENDIF()

IF(NOT EXISTS ${PROJECT_BINARY_DIR}/bin/res)
	# Unpack resources
	message("Unpacking resources ...")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xzvf ${RES_ARCHIVE_PATH}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
ENDIF()


# Set up project structure

set(CONTRIB_PREFIX contrib)

set(Recast_INCLUDE_DIR ${CONTRIB_PREFIX}/recast_navigation/Recast/Include)
FILE(GLOB Recast_SOURCE_FILES ${CONTRIB_PREFIX}/recast_navigation/Recast/Source/*.cpp)
FILE(GLOB Recast_INCLUDE_FILES ${CONTRIB_PREFIX}/recast_navigation/Recast/Include/*.h)
add_library(Recast OBJECT ${Recast_INCLUDE_FILES} ${Recast_SOURCE_FILES})

set(Detour_INCLUDE_DIR ${CONTRIB_PREFIX}/recast_navigation/Detour/Include)
FILE(GLOB Detour_SOURCE_FILES ${CONTRIB_PREFIX}/recast_navigation/Detour/Source/*.cpp)
FILE(GLOB Detour_INCLUDE_FILES ${CONTRIB_PREFIX}/recast_navigation/Detour/Include/*.h)
add_library(Detour OBJECT ${Detour_INCLUDE_FILES} ${Detour_SOURCE_FILES})

set(DetourTileCache_INCLUDE_DIR ${CONTRIB_PREFIX}/recast_navigation/DetourTileCache/Include)
FILE(GLOB DetourTileCache_SOURCE_FILES ${CONTRIB_PREFIX}/recast_navigation/DetourTileCache/Source/*.cpp)
FILE(GLOB DetourTileCache_INCLUDE_FILES ${CONTRIB_PREFIX}/recast_navigation/DetourTileCache/Include/*.h)
add_library(DetourTileCache OBJECT ${DetourTileCache_INCLUDE_FILES} ${DetourTileCache_SOURCE_FILES})

set(DetourCrowd_INCLUDE_DIR ${CONTRIB_PREFIX}/recast_navigation/DetourCrowd/Include)
FILE(GLOB DetourCrowd_SOURCE_FILES ${CONTRIB_PREFIX}/recast_navigation/DetourCrowd/Source/*.cpp)
FILE(GLOB DetourCrowd_INCLUDE_FILES ${CONTRIB_PREFIX}/recast_navigation/DetourCrowd/Include/*.h)
add_library(DetourCrowd OBJECT ${DetourCrowd_INCLUDE_FILES} ${DetourCrowd_SOURCE_FILES})

set(DebugUtils_INCLUDE_DIR ${CONTRIB_PREFIX}/recast_navigation/DebugUtils/Include)
FILE(GLOB DebugUtils_SOURCE_FILES ${CONTRIB_PREFIX}/recast_navigation/DebugUtils/Source/*.cpp)
FILE(GLOB DebugUtils_INCLUDE_FILES ${CONTRIB_PREFIX}/recast_navigation/DebugUtils/Include/*.h)
add_library(DebugUtils OBJECT ${DebugUtils_INCLUDE_FILES} ${DebugUtils_SOURCE_FILES})

set(minizip_INCLUDE_DIR ${CONTRIB_PREFIX}/minizip)
FILE(GLOB minizip_FILES ${CONTRIB_PREFIX}/minizip/*.*)
add_library(minizip OBJECT ${minizip_FILES})

set(nanovg_INCLUDE_DIR ${CONTRIB_PREFIX}/nanovg)
FILE(GLOB nanovg_FILES ${CONTRIB_PREFIX}/nanovg/*.*)
add_library(nanovg OBJECT ${nanovg_FILES})


# Required packages
find_package(OpenGL REQUIRED)

# Dependencies and include locations
include_directories(include)
include_directories(bin/res/shaders)
include_directories(${CONTRIB_PREFIX})
include_directories(${CONTRIB_PREFIX}/minizip)

include_directories(${Recast_INCLUDE_DIR})
include_directories(${Detour_INCLUDE_DIR})
include_directories(${DetourTileCache_INCLUDE_DIR})
include_directories(${DetourCrowd_INCLUDE_DIR})
include_directories(${DebugUtils_INCLUDE_DIR})
include_directories(${nanovg_INCLUDE_DIR})

IF(LocalIncludes)
	include_directories(${CONTRIB_PREFIX}/SDL2)
	include_directories(${CONTRIB_PREFIX}/assimp)
	include_directories(${CONTRIB_PREFIX}/GL)
	include_directories(${CONTRIB_PREFIX}/glm)
ELSE(LocalIncludes)
	find_package(SDL2 		REQUIRED)
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_ttf 	REQUIRED)
	find_package(assimp 	REQUIRED)
	find_package(ZLIB 		REQUIRED)
	find_package(GLEW 		REQUIRED)
	find_package(glm 		REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIR})
	include_directories(${SDL2_INCLUDE_DIR})
	include_directories(${assimp_INCLUDE_DIRS})
	include_directories(${ZLIB_INCLUDE_DIRS})
	include_directories(${GLEW_INCLUDE_DIRS})
	include_directories(${GLM_INCLUDE_DIRS})
ENDIF(LocalIncludes)

# Project headers
file(GLOB HEADER_FILES1 include/*.h)
file(GLOB HEADER_FILES2 include/*.hpp)

# Project Sources
file(GLOB_RECURSE SOURCE_FILES1 src/*.c)
file(GLOB_RECURSE SOURCE_FILES2 src/*.cpp)
file(GLOB_RECURSE SHADER_FILES bin/res/shaders/*.*)

add_executable(
  ${PROJECT_NAME} 
  ${HEADER_FILES1} 
  ${HEADER_FILES2} 
  ${SOURCE_FILES1} 
  ${SOURCE_FILES2} 
  ${SHADER_FILES} 
  $<TARGET_OBJECTS:Recast> 
  $<TARGET_OBJECTS:Detour>
  $<TARGET_OBJECTS:DetourTileCache>
  $<TARGET_OBJECTS:DetourCrowd>
  $<TARGET_OBJECTS:DebugUtils>
  $<TARGET_OBJECTS:minizip>
  $<TARGET_OBJECTS:nanovg>)

IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
		lib/SDL2
  		lib/SDL2main
  		lib/SDL2_image
  		lib/SDL2_ttf
  		lib/assimp
  		lib/zdll
  		lib/glew32
  		${OPENGL_LIBRARIES})
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
		${SDL2_LIBRARY}
	  	${SDL2_IMAGE_LIBRARY}
	  	${SDL2_TTF_LIBRARY}
	  	${assimp_LIBRARIES}
	  	${ZLIB_LIBRARY}
	  	${OPENGL_LIBRARIES}
		${GLEW_LIBRARIES}
        ${FILESYSTEM_LIB})
ENDIF(WIN32)
