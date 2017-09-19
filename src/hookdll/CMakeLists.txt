cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(hookdll VERSION 0.1 LANGUAGES CXX)
set(library_name "hookdll")

## Files ##
# Main
SET(HDRS
    include/${library_name}/apihook.h
    include/${library_name}/disasm.h
    include/${library_name}/disasm_table.h
    include/${library_name}/dllmain.h
    include/${library_name}/fallout3info.h
    include/${library_name}/falloutnvinfo.h
    include/${library_name}/gameinfo.h
    include/${library_name}/hooklock.h
    include/${library_name}/logger.h
    include/${library_name}/modinfo.h
    include/${library_name}/oblivioninfo.h
    include/${library_name}/obse.h
    include/${library_name}/profile.h
    include/${library_name}/reroutes.h
    include/${library_name}/skyriminfo.h
    include/${library_name}/utility.h
)

SET(SRCS
    src/apihook.cpp
    src/disasm.cpp
    src/disasm_table.cpp
    src/dllmain.cpp
    src/fallout3info.cpp
    src/falloutnvinfo.cpp
    src/gameinfo.cpp
    src/hooklock.cpp
    src/logger.cpp
    src/modinfo.cpp
    src/oblivioninfo.cpp
    src/process_blacklist.txt
    src/profile.cpp
    src/skyriminfo.cpp
    src/utility.cpp
    #
    src/version.rc
)

## Library ##
add_library(${library_name} SHARED ${HDRS} ${SRCS})
set_target_properties(${library_name} PROPERTIES OUTPUT_NAME "hook")
add_definitions(-DDEBUG_LOG)
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC common MOShared fmt::fmt-header-only Shlwapi Mincore)
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








