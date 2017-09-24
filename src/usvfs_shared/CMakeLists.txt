cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(usvfs_shared VERSION 0.1 LANGUAGES CXX)
set(library_name "usvfs_shared")

## Files ##
# Main
SET(shared_HDRS
    include/${library_name}/addrtools.h
    include/${library_name}/logging.h
    include/${library_name}/windows_error.h
    include/${library_name}/debug_monitor.h
    include/${library_name}/shmlogger.h
    include/${library_name}/directory_tree.h
    include/${library_name}/wildcard.h
    include/${library_name}/winapi.h
    include/${library_name}/shared_memory.h
    include/${library_name}/stringcast.h
    include/${library_name}/stringcast_basic.h
    include/${library_name}/stringcast_win.h
    include/${library_name}/stringutils.h
    include/${library_name}/scopeguard.h
    include/${library_name}/exceptionex.h
    include/${library_name}/ntdll_declarations.h
    include/${library_name}/unicodestring.h
)

SET(shared_SRCS
    src/addrtools.cpp
    src/logging.cpp
    src/windows_error.cpp
    src/debug_monitor.cpp
    src/shmlogger.cpp
    src/directory_tree.cpp
    src/wildcard.cpp
    src/winapi.cpp
    src/stringutils.cpp
    src/stringcast_win.cpp
    src/scopeguard.cpp
    src/exceptionex.cpp
    src/ntdll_declarations.cpp
    src/unicodestring.cpp
)

## Library ##
add_library(${library_name} STATIC ${shared_SRCS} ${shared_HDRS})
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC fmt::fmt-header-only spdlog common Boost::boost)
ADD_DEFINITIONS(-DSPDLOG_NO_NAME -DSPDLOG_NO_REGISTRY_MUTEX)
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
set_target_properties(${library_name}Tests PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
target_include_directories(${library_name}Tests PRIVATE test)
target_link_libraries(${library_name}Tests PRIVATE ${library_name} spdlog GTest fmt::fmt-header-only)
add_test(${library_name}_Tests Tests)





