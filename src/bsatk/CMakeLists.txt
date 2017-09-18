cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(bsatk VERSION 0.1 LANGUAGES CXX)
set(library_name "bsatk")

## Files ##
# Main
SET(HDRS
    include/${library_name}/filehash.h
    include/${library_name}/errorcodes.h
    include/${library_name}/bsatypes.h
    include/${library_name}/bsafile.h
    include/${library_name}/bsafolder.h
    include/${library_name}/bsaexception.h
    include/${library_name}/bsaarchive.h
)

SET(SRCS
    src/filehash.cpp
    src/bsafile.cpp
    src/bsaexception.cpp
    src/bsafolder.cpp
    src/bsaarchive.cpp
    src/bsatypes.cpp
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
target_link_libraries(${library_name} PUBLIC Zlib common)
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








