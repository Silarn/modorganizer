cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(archive VERSION 0.1 LANGUAGES CXX)
set(library_name "archive")

# https://github.com/iaz3/modorganizer-archive/blob/master/src/CMakeLists.txt

## Files ##
# Main
SET(HDRS
    include/${library_name}/archive.h
    include/${library_name}/callback.h
    include/${library_name}/extractcallback.h
    include/${library_name}/opencallback.h
    include/${library_name}/multioutputstream.h
    include/${library_name}/propertyvariant.h
    include/${library_name}/inputstream.h
    include/${library_name}/unknown_impl.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/IDecl.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/IPassword.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/IStream.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/IProgress.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/PropID.h
    # ${SEVENZIP_ROOT_DIR}/CPP/7zip/Archive/IArchive.h
    # ${SEVENZIP_ROOT_DIR}/CPP/Common/MyUnknown.h
)

SET(SRCS
    src/archive.cpp
    src/extractcallback.cpp
    src/callback.cpp
    src/opencallback.cpp
    src/multioutputstream.cpp
    src/interfaceguids.cpp
    src/propertyvariant.cpp
    src/inputstream.cpp
)

## Dependencies ##
find_package(Qt5Core REQUIRED)

## Library ##
add_library(${library_name} SHARED ${HDRS} ${SRCS})
add_definitions(-DARCHIVE_EXPORT)
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC common Qt5::Core 7zip)
set_target_properties(${library_name} PROPERTIES AUTOMOC ON)

add_custom_command(TARGET ${library_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${library_name}>/dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${library_name}>/archive.dll
        $<TARGET_FILE_DIR:${library_name}>/dlls/archive.dll
)
# Install Targets
install(TARGETS ${library_name} EXPORT ${library_name}Config
    ARCHIVE  DESTINATION lib # <prefix>/lib. DLL import libs.
    LIBRARY  DESTINATION lib # <prefix>/lib. Shared Libs, non DLL.
    RUNTIME  DESTINATION bin/dlls # <prefix>/bin. EXEs, DLLs and shit.
)
install(DIRECTORY include/ DESTINATION include)
INSTALL(FILES $<TARGET_FILE_DIR:${library_name}>/archive.pdb DESTINATION pdb)
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








