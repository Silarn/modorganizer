cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(game_gamebryo VERSION 0.1 LANGUAGES CXX)
set(library_name "game_gamebryo")

## Files ##
# Main
SET(HDRS
    include/${library_name}/gamegamebryo.h
    include/${library_name}/dummybsa.h
    include/${library_name}/gamebryobsainvalidation.h
    include/${library_name}/gamebryodataarchives.h
    include/${library_name}/gamebryoscriptextender.h
    include/${library_name}/gamebryosavegame.h
    include/${library_name}/gamebryolocalsavegames.h
    include/${library_name}/gamebryosavegameinfo.h
    include/${library_name}/gamebryosavegameinfowidget.h
    include/${library_name}/gamebryogameplugins.h
    include/${library_name}/gamebryounmanagedmods.h
)

SET(SRCS
    src/gamegamebryo.cpp
    src/dummybsa.cpp
    src/gamebryobsainvalidation.cpp
    src/gamebryodataarchives.cpp
    src/gamebryoscriptextender.cpp
    src/gamebryosavegame.cpp
    src/gamebryolocalsavegames.cpp
    src/gamebryosavegameinfo.cpp
    src/gamebryosavegameinfowidget.cpp
    src/gamebryogameplugins.cpp
    src/gamebryounmanagedmods.cpp
)

SET(QT
    src/gamebryosavegameinfowidget.ui
)

source_group(QT FILES ${QT})

## Dependencies ##
find_package(Qt5Widgets REQUIRED)

## Library ##
add_library(${library_name} STATIC ${HDRS} ${SRCS})
set_target_properties(${library_name} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC common uibase gamefeatures Qt5::Widgets Version lz4
)
# Install Targets
install(TARGETS ${library_name} EXPORT ${library_name}Config
    ARCHIVE  DESTINATION lib # <prefix>/lib. DLL import libs.
    LIBRARY  DESTINATION lib # <prefix>/lib. Shared Libs, non DLL.
    RUNTIME  DESTINATION bin/plugins # <prefix>/bin. EXEs, DLLs and shit.
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








