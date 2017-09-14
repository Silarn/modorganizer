cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(thooklib VERSION 0.1 LANGUAGES CXX)
set(library_name "thooklib")

## Files ##
# Main
SET(thooklib_HDRS
    include/${library_name}/asmjit_sane.h
    include/${library_name}/utility.h
    include/${library_name}/hooklib.h
    include/${library_name}/udis86wrapper.h
    include/${library_name}/ttrampolinepool.h
)

SET(thooklib_SRCS
    src/utility.cpp
    src/hooklib.cpp
    src/udis86wrapper.cpp
    src/ttrampolinepool.cpp
)

## Library ##
add_library(${library_name} STATIC ${thooklib_HDRS} ${thooklib_SRCS})
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC spdlog usvfs_shared AsmJit UDIS86)
# Install Targets
install(TARGETS ${library_name} EXPORT ${library_name}Config
    ARCHIVE  DESTINATION lib # <prefix>/lib. DLL import libs.
    LIBRARY  DESTINATION lib # <prefix>/lib. Shared Libs, non DLL.
    RUNTIME  DESTINATION bin # <prefix>/bin. EXEs, DLLs and shit.
)
install(DIRECTORY include/ DESTINATION include)
# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
install(EXPORT ${library_name}Config DESTINATION cmake)
# This makes the project importable from the build directory
export(TARGETS ${library_name} FILE ${library_name}Config.cmake)

## Tests ##
add_executable(${library_name}Tests EXCLUDE_FROM_ALL
    test/main.cpp
)
target_include_directories(${library_name}Tests PRIVATE test)
target_link_libraries(${library_name}Tests PRIVATE ${library_name} spdlog GTest)
add_test(${library_name}_Tests Tests)








