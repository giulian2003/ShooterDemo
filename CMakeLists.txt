project(ShooterDemo)

cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR})

# CMAKE extensions
set(CMAKE_MODULE_PATH cmake_modules)

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

set(Recast_INCLUDE_DIR contrib/Recast/Include)
FILE(GLOB Recast_SOURCE_FILES contrib/Recast/Source/*.cpp)
FILE(GLOB Recast_INCLUDE_FILES contrib/Recast/Include/*.h)
add_library(Recast OBJECT ${Recast_INCLUDE_FILES} ${Recast_SOURCE_FILES})

set(Detour_INCLUDE_DIR contrib/Detour/Include)
FILE(GLOB Detour_SOURCE_FILES contrib/Detour/Source/*.cpp)
FILE(GLOB Detour_INCLUDE_FILES contrib/Detour/Include/*.h)
add_library(Detour OBJECT ${Detour_INCLUDE_FILES} ${Detour_SOURCE_FILES})

set(DetourTileCache_INCLUDE_DIR contrib/DetourTileCache/Include)
FILE(GLOB DetourTileCache_SOURCE_FILES contrib/DetourTileCache/Source/*.cpp)
FILE(GLOB DetourTileCache_INCLUDE_FILES contrib/DetourTileCache/Include/*.h)
add_library(DetourTileCache OBJECT ${DetourTileCache_INCLUDE_FILES} ${DetourTileCache_SOURCE_FILES})

set(DetourCrowd_INCLUDE_DIR contrib/DetourCrowd/Include)
FILE(GLOB DetourCrowd_SOURCE_FILES contrib/DetourCrowd/Source/*.cpp)
FILE(GLOB DetourCrowd_INCLUDE_FILES contrib/DetourCrowd/Include/*.h)
add_library(DetourCrowd OBJECT ${DetourCrowd_INCLUDE_FILES} ${DetourCrowd_SOURCE_FILES})

set(DebugUtils_INCLUDE_DIR contrib/DebugUtils/Include)
FILE(GLOB DebugUtils_SOURCE_FILES contrib/DebugUtils/Source/*.cpp)
FILE(GLOB DebugUtils_INCLUDE_FILES contrib/DebugUtils/Include/*.h)
add_library(DebugUtils OBJECT ${DebugUtils_INCLUDE_FILES} ${DebugUtils_SOURCE_FILES})

set(minizip_INCLUDE_DIR contrib/minizip)
FILE(GLOB minizip_FILES contrib/minizip/*.*)
add_library(minizip OBJECT ${minizip_FILES})

set(nanovg_INCLUDE_DIR contrib/nanovg)
FILE(GLOB nanovg_FILES contrib/nanovg/*.*)
add_library(nanovg OBJECT ${nanovg_FILES})

# Required packages
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2TTF REQUIRED)
find_package(SDL2IMAGE REQUIRED)
find_package(GLEW REQUIRED)
find_package(ZLIB REQUIRED)
find_package(assimp REQUIRED)

# Dependencies and include locations
include_directories(include)
include_directories(bin/res/shaders)
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2TTF_INCLUDE_DIR})
include_directories(${SDL2IMAGE_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})
include_directories(${Recast_INCLUDE_DIR})
include_directories(${Detour_INCLUDE_DIR})
include_directories(${DetourTileCache_INCLUDE_DIR})
include_directories(${DetourCrowd_INCLUDE_DIR})
include_directories(${DebugUtils_INCLUDE_DIR})
include_directories(${NavMesh_INCLUDE_DIR})
include_directories(${minizip_INCLUDE_DIR})
include_directories(${nanovg_INCLUDE_DIR})

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

TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
  ${GLEW_LIBRARY}
  ${SDL2_LIBRARY}
  ${SDL2TTF_LIBRARY}
  ${SDL2IMAGE_LIBRARY}
  ${ZLIB_LIBRARY}
  ${ASSIMP_LIBRARIES}
  ${OPENGL_LIBRARIES})
