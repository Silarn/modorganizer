cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(helper VERSION 0.1 LANGUAGES CXX)
set(library_name "helper")

## Files ##
# Main
SET(HDRS
    include/${library_name}/privileges.h
)

SET(SRCS
    src/main.cpp
    src/privileges.cpp
)

## Dependencies ##
find_package(Qt5Core REQUIRED)

## Library ##
add_executable(${library_name} ${HDRS} ${SRCS})
set_target_properties(${library_name} PROPERTIES AUTOMOC ON)
SET_TARGET_PROPERTIES(${library_name} PROPERTIES LINK_FLAGS "/manifestuac:level='requireAdministrator'")
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC common Qt5::Core)
# Install Targets
install(TARGETS ${library_name} EXPORT ${library_name}Config
    ARCHIVE  DESTINATION lib # <prefix>/lib. DLL import libs.
    LIBRARY  DESTINATION lib # <prefix>/lib. Shared Libs, non DLL.
    RUNTIME  DESTINATION bin # <prefix>/bin. EXEs, DLLs and shit.
)
INSTALL(FILES $<TARGET_FILE_DIR:${library_name}>/${library_name}.pdb
        DESTINATION pdb
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








