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
set(GENERAL_OUTPUT_DIR CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE)

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

## Imported Targets ##
# Qt
if (IS_X64)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.1/msvc2017_64")
else()
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.1/msvc2015")
endif()
find_package(Qt5Core REQUIRED)

# ZLIB
set(ZLIB_INCLUDE_DIRS ${PREFIX_PATH}/include)
set(ZLIB_LIBRARY ${PREFIX_PATH}/lib/zlibstatic${CMAKE_STATIC_LIBRARY_SUFFIX})
set(ZLIB_LIBRARY_D ${PREFIX_PATH}/lib/zlibstaticd${CMAKE_STATIC_LIBRARY_SUFFIX})
if(NOT EXISTS ${ZLIB_INCLUDE_DIRS}/zlib.h OR NOT EXISTS ${ZLIB_LIBRARY})
    message(FATAL_ERROR "Zlib not found, have you built it correctly?\n"
            "${library_name} requires Static Zlib")
endif()
add_library(Zlib STATIC IMPORTED)
set_property(TARGET Zlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIRS})
set_property(TARGET Zlib PROPERTY IMPORTED_LOCATION_RELEASE ${ZLIB_LIBRARY})
set_property(TARGET Zlib PROPERTY IMPORTED_LOCATION_DEBUG ${ZLIB_LIBRARY_D})
# Fmtlib
find_package(fmt REQUIRED CONFIG)
# spdlog
set(SPDLog_INCLUDE_DIRS ${PREFIX_PATH}/include)
if(NOT EXISTS ${SPDLog_INCLUDE_DIRS}/spdlog/spdlog.h)
    message(FATAL_ERROR "spdlog not found, have you installed it correctly?")
endif()
add_library(spdlog INTERFACE IMPORTED)
set_property(TARGET spdlog PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${spdlog_INCLUDE_DIRS})
# Google Test
set(GTEST_INCLUDE_DIRS ${PREFIX_PATH}/include)
set(GTEST_LIBRARY ${PREFIX_PATH}/lib/gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GTEST_LIBRARY_D ${PREFIX_PATH}/lib/gtestd${CMAKE_STATIC_LIBRARY_SUFFIX})
if(NOT EXISTS ${GTEST_INCLUDE_DIRS}/gtest/gtest.h OR NOT EXISTS ${GTEST_LIBRARY})
    message(FATAL_ERROR "GTest not found, have you built it correctly?\n"
            "${library_name} requires Static GTest")
endif()
add_library(GTest STATIC IMPORTED)
set_property(TARGET GTest PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS})
set_property(TARGET GTest PROPERTY IMPORTED_LOCATION_RELEASE ${GTEST_LIBRARY})
set_property(TARGET GTest PROPERTY IMPORTED_LOCATION_DEBUG ${GTEST_LIBRARY_D})
# AsmJit
set(ASMJIT_INCLUDE_DIRS ${PREFIX_PATH}/include)
set(ASMJIT_LIBRARY ${PREFIX_PATH}/lib/asmjit${CMAKE_STATIC_LIBRARY_SUFFIX})
set(ASMJIT_LIBRARY_D ${PREFIX_PATH}/lib/asmjitd${CMAKE_STATIC_LIBRARY_SUFFIX})
if(NOT EXISTS ${ASMJIT_INCLUDE_DIRS}/asmjit/asmjit.h OR NOT EXISTS ${ASMJIT_LIBRARY})
    message(FATAL_ERROR "AsmJit not found, have you built it correctly?\n"
            "${library_name} requires Static AsmJit")
endif()
add_library(AsmJit STATIC IMPORTED)
set_property(TARGET AsmJit PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ASMJIT_INCLUDE_DIRS})
set_property(TARGET AsmJit PROPERTY IMPORTED_LOCATION_RELEASE ${ASMJIT_LIBRARY})
set_property(TARGET AsmJit PROPERTY IMPORTED_LOCATION_DEBUG ${ASMJIT_LIBRARY_D})
# UDIS86
set(UDIS86_INCLUDE_DIRS ${PREFIX_PATH}/include)
set(UDIS86_LIBRARY ${PREFIX_PATH}/lib/libudis86${CMAKE_STATIC_LIBRARY_SUFFIX})
set(UDIS86_LIBRARY_D ${PREFIX_PATH}/lib/libudis86d${CMAKE_STATIC_LIBRARY_SUFFIX})
if(NOT EXISTS ${UDIS86_INCLUDE_DIRS}/udis86.h OR NOT EXISTS ${UDIS86_LIBRARY})
    message(FATAL_ERROR "UDIS86 not found, have you built it correctly?\n"
            "${library_name} requires Static UDIS86")
endif()
add_library(UDIS86 STATIC IMPORTED)
set_property(TARGET UDIS86 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${UDIS86_INCLUDE_DIRS})
set_property(TARGET UDIS86 PROPERTY IMPORTED_LOCATION_RELEASE ${UDIS86_LIBRARY})
set_property(TARGET UDIS86 PROPERTY IMPORTED_LOCATION_DEBUG ${UDIS86_LIBRARY_D})
# bit7z
set(bit7z_INCLUDE_DIRS ${PREFIX_PATH}/include)
set(bit7z_LIBRARY ${PREFIX_PATH}/lib/bit7z64${CMAKE_STATIC_LIBRARY_SUFFIX})
set(bit7z_LIBRARY_D ${PREFIX_PATH}/lib/bit7z64_d${CMAKE_STATIC_LIBRARY_SUFFIX})
if(NOT EXISTS ${bit7z_INCLUDE_DIRS}/bit7z.h OR NOT EXISTS ${bit7z_LIBRARY})
    message(FATAL_ERROR "bit7z not found, have you built it correctly?\n"
            "${library_name} requires Static bit7z")
endif()
add_library(bit7z STATIC IMPORTED)
set_property(TARGET bit7z PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${bit7z_INCLUDE_DIRS})
set_property(TARGET bit7z PROPERTY IMPORTED_LOCATION_RELEASE ${bit7z_LIBRARY})
set_property(TARGET bit7z PROPERTY IMPORTED_LOCATION_DEBUG ${bit7z_LIBRARY_D})

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










