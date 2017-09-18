# Top Level CMake for USVFS
cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(ModOrganizer VERSION 0.1 LANGUAGES CXX)
set(library_name "Mod Organizer")

## Sanity ##
if(NOT WIN32)
    message(FATAL_ERROR "Unsupported platform.")
endif()
set(IS_X64 0)
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set(IS_X64 1)
endif()
message(STATUS "Using x64: " ${IS_X64})

## Config ##
list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES "MinSizeRel" "RelWithDebInfo")
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "" FORCE)
message(STATUS "Supported Configuartions: ${CMAKE_CONFIGURATION_TYPES}")

# PREFIX_PATH is a single directory for common third party libs
# It will be the first path in CMAKE_PREFIX_PATH
set(PREFIX_PATH "C:/_Diana/Dev/C++/- Libraries" CACHE PATH "Root directory for CMAKE_PREFIX_PATH.\nShould contain two directories, x64 and x86, with third party dependencies located there respectively.")
if(IS_X64)
    set(PREFIX_PATH ${PREFIX_PATH}/x64)
else()
    set(PREFIX_PATH ${PREFIX_PATH}/x86)
endif()
set(CMAKE_PREFIX_PATH ${PREFIX_PATH} CACHE PATH "" FORCE)
if(NOT EXISTS ${PREFIX_PATH})
    message(FATAL_ERROR "The path \"${PREFIX_PATH}\" doesnt exist!\nDid you set PREFIX_PATH correctly?")
endif()

# Unified output.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
#
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set(GENERAL_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

## C++ Compiler Stuff ##
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
# MSVC only since CMake doesnt support MSVC properly
if (MSVC)
    if (MSVC_VERSION LESS 1910)
        message(WARNING "Using old MSVC Version ${MSVC_VERSION}, compiler flags may not be accurate!\n"
                        "Only tested with Visual Studio 2017 15.3")
    endif()
    ## For all configurations ##
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE -DASMJIT_STATIC)
    # For C++17, Standards Conformance, and Proper Warning Levels.
    set(COMPILER
        "/std:c++17"
        "/Zc:inline"
        "/Zc:referenceBinding"
        "/Zc:rvalueCast"
        "/Zc:strictStrings"
        "/Zc:throwingNew"
        "/W4"
        "/permissive-"
    )
    string(REPLACE ";" " " COMPILER "${COMPILER}")

    set(LINKER
        "/DEBUG:FASTLINK"
    )
    string(REPLACE ";" " " LINKER "${LINKER}")
    
    set(COMPILER_DEBUG
    )
    string(REPLACE ";" " " COMPILER_DEBUG "${COMPILER_DEBUG}")
    
    set(COMPILER_RELEASE
        "/Zi" # Improves debugging and doesnt affect optimizations.
        "/GL" # Whole Program Optimization
        "/Gy" # Function Level Linking
        "/Gw" # Optimize Global Data
        "/Qpar" # Auto Parralize Loops
        "/Qpar-report:1" # Outputs an info message for loops that are parallelized.
        "/Qvec-report:1" # Outputs an info message for loops that are vectorized.
    )
    string(REPLACE ";" " " COMPILER_RELEASE "${COMPILER_RELEASE}")
    
    set(LINKER_DEBUG
        "/SAFESEH:NO"
    )
    string(REPLACE ";" " " LINKER_DEBUG "${LINKER_DEBUG}")
    
    set(LINKER_RELEASE
        "/INCREMENTAL:NO" # Disable Incremental Linking
        "/LTCG" # Link Time Code Generation.
        "/OPT:REF" # Remove unused references
    )
    if(IS_X64)
        list(APPEND LINKER_RELEASE "/OPT:ICF") # Recomended for 64bit
    endif()
    string(REPLACE ";" " " LINKER_RELEASE "${LINKER_RELEASE}")
    
    ## Store Originals
    set(_CMAKE_EXE_LINKER_FLAGS 			${CMAKE_EXE_LINKER_FLAGS} CACHE STRING "")
    set(_CMAKE_EXE_LINKER_FLAGS_DEBUG 		${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "")
    set(_CMAKE_EXE_LINKER_FLAGS_RELEASE 	${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "")
    #
    set(_CMAKE_SHARED_LINKER_FLAGS 			${CMAKE_SHARED_LINKER_FLAGS} CACHE STRING "")
    set(_CMAKE_SHARED_LINKER_FLAGS_DEBUG 	${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "")
    set(_CMAKE_SHARED_LINKER_FLAGS_RELEASE 	${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "")
    #
    set(_CMAKE_STATIC_LINKER_FLAGS 			${CMAKE_STATIC_LINKER_FLAGS} CACHE STRING "")
    set(_CMAKE_STATIC_LINKER_FLAGS_DEBUG 	${CMAKE_STATIC_LINKER_FLAGS_DEBUG} CACHE STRING "")
    set(_CMAKE_STATIC_LINKER_FLAGS_RELEASE 	${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING "")
    ## Set Linker Flags
    # Note: So many different linker flags,,,
    # LINK_FLAGS can replace below?
    # CMAKE_EXE_LINKER_FLAGS
    # CMAKE_MODULE_LINKER_FLAGS
    # CMAKE_SHARED_LINKER_FLAGS
    # CMAKE_STATIC_LINKER_FLAGS
    ##
    set(CMAKE_EXE_LINKER_FLAGS 				"${_CMAKE_EXE_LINKER_FLAGS} ${LINKER}" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG 		"${_CMAKE_EXE_LINKER_FLAGS_DEBUG} ${LINKER_DEBUG}" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE 		"${_CMAKE_EXE_LINKER_FLAGS_RELEASE} ${LINKER_RELEASE}" CACHE STRING "" FORCE)
    #
    set(CMAKE_SHARED_LINKER_FLAGS 			"${_CMAKE_SHARED_LINKER_FLAGS} ${LINKER}" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG 	"${_CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${LINKER_DEBUG}" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE 	"${_CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${LINKER_RELEASE}" CACHE STRING "" FORCE)
    #
    set(CMAKE_STATIC_LINKER_FLAGS 			"${_CMAKE_STATIC_LINKER_FLAGS} ${LINKER}" CACHE STRING "" FORCE)
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG 	"${_CMAKE_STATIC_LINKER_FLAGS_DEBUG} ${LINKER_DEBUG}" CACHE STRING "" FORCE)
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE 	"${_CMAKE_STATIC_LINKER_FLAGS_RELEASE} ${LINKER_RELEASE}" CACHE STRING "" FORCE)
    ##
    
    ## Store Originals
    set(_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING "")
    set(_CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} CACHE STRING "")
    set(_CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "")
    ## Set Compiler Flags
    set(CMAKE_CXX_FLAGS "${_CMAKE_CXX_FLAGS} ${COMPILER}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "${_CMAKE_CXX_FLAGS_DEBUG} ${COMPILER_DEBUG}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${_CMAKE_CXX_FLAGS_RELEASE} ${COMPILER_RELEASE}" CACHE STRING "" FORCE)
else()
    message(WARNING "Unsupported or unknown compiler")
endif()

## Helper Function ##
#if
# includeDir is where obvious.
# libraryName will be the name of the imported target.
# libraryFile is the name of the library on disk, IE zlib.dll
# includeFile will be the path relative to includeDir
# type will be STATIC or SHARED or INTERFACE or the like.
function(import_target libraryName libraryFile includeDir includeFile type)
    set(INCLUDE_DIRS ${includeDir})
    set(LIBRARY ${PREFIX_PATH}/lib/${libraryFile}${CMAKE_${type}_LIBRARY_SUFFIX})
    set(LIBRARY_D ${PREFIX_PATH}/lib/${libraryFile}d${CMAKE_${type}_LIBRARY_SUFFIX})
    if(NOT EXISTS ${INCLUDE_DIRS}/${includeFile} OR NOT type STREQUAL "INTERFACE" AND NOT EXISTS ${LIBRARY} OR NOT EXISTS ${LIBRARY_D})
        message(FATAL_ERROR "${libraryName} not found, have you installed it correctly?\n"
                "${library_name} requires ${libraryName}")
    endif()
    add_library(${libraryName} ${type} IMPORTED)
    set_property(TARGET ${libraryName} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
    if(NOT type STREQUAL "INTERFACE")
        set_property(TARGET ${libraryName} PROPERTY IMPORTED_LOCATION_RELEASE ${LIBRARY})
        set_property(TARGET ${libraryName} PROPERTY IMPORTED_LOCATION_DEBUG ${LIBRARY_D})
    endif()
endfunction()
#endif

## Imported Targets ##
# Qt
if (IS_X64)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.1/msvc2017_64")
else()
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.1/msvc2015")
endif()
find_package(Qt5Core REQUIRED)

# ZLIB
import_target(Zlib "zlibstatic" ${PREFIX_PATH}/include "zlib.h" "STATIC")
# Fmtlib
find_package(fmt REQUIRED CONFIG)
# spdlog
import_target(spdlog "" ${PREFIX_PATH}/include "spdlog/spdlog.h" "INTERFACE")
# Google Test
import_target(GTest "gtest" ${PREFIX_PATH}/include "gtest/gtest.h" "STATIC")
# AsmJit
import_target(AsmJit "asmjit" ${PREFIX_PATH}/include "asmjit/asmjit.h" "STATIC")
# UDIS86
import_target(UDIS86 "libudis86" ${PREFIX_PATH}/include "udis86.h" "STATIC")
# bit7z
import_target(bit7z "bit7z" ${PREFIX_PATH}/include "bit7z.hpp" "STATIC")
## Targets ##
# add_subdirectory(usvfs_shared)
# add_subdirectory(thooklib)
# add_subdirectory(tinjectlib)
# add_subdirectory(usvfs)
##
add_subdirectory(common)
add_subdirectory(bsatk)
add_subdirectory(uibase)
add_subdirectory(archive)
add_subdirectory(gamefeatures)
add_subdirectory(MO)
add_subdirectory(hookdll)










