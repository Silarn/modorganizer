## Common Library.
cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(common VERSION 0.1 LANGUAGES CXX)
set(library_name "common")

## Files ##
# Main
SET(HDRS
    include/${library_name}/dllmain.h # Handle Static vs Shared DLL exports.
    include/${library_name}/sane_windows.h # A sane windows.h import.
    include/${library_name}/fwd_windows.h # So you can use windows.h types without windows.h.
)

SET(SRCS
    src/apihook.cpp
)

## Library ##
add_library(${library_name} STATIC ${HDRS} ${SRCS})
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC fmt::fmt-header-only)
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
# add_executable(${library_name}Tests EXCLUDE_FROM_ALL
    # test/main.cpp
# )
# target_include_directories(${library_name}Tests PRIVATE test)
# target_link_libraries(${library_name}Tests PRIVATE ${library_name} spdlog GTest)
# add_test(${library_name}_Tests Tests)








