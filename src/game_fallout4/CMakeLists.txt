cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(game_fallout4 VERSION 0.1 LANGUAGES CXX)
set(library_name "game_fallout4")

## Files ##
# Main
SET(HDRS
    include/${library_name}/fallout4dataarchives.h
    include/${library_name}/fallout4gameplugins.h
    include/${library_name}/fallout4savegame.h
    include/${library_name}/fallout4savegameinfo.h
    include/${library_name}/fallout4scriptextender.h
    include/${library_name}/fallout4unmanagedmods.h
    include/${library_name}/gamefallout4.h
)

SET(SRCS
    src/fallout4dataarchives.cpp
    src/fallout4gameplugins.cpp
    src/fallout4savegame.cpp
    src/fallout4savegameinfo.cpp
    src/fallout4scriptextender.cpp
    src/fallout4unmanagedmods.cpp
    src/gamefallout4.cpp
)

SET(QT
    src/fallout4.qrc
    src/splash.png
    src/game_fallout4_en.ts
    src/gamefallout4.json
)

source_group(QT FILES ${QT})

## Dependencies ##
find_package(Qt5Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

## Library ##
add_library(${library_name} SHARED ${HDRS} ${SRCS} ${QT})
set_target_properties(${library_name} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
QT5_CREATE_TRANSLATION(${library_name}_translations_qm ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/${library_name}_en.ts)
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC common uibase gamefeatures game_gamebryo Qt5::Widgets lz4)
add_custom_command(TARGET ${library_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${library_name}>/plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${library_name}>/${library_name}.dll
        $<TARGET_FILE_DIR:${library_name}>/plugins/${library_name}.dll	
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:lz4>/liblz4.dll
        $<TARGET_FILE_DIR:${library_name}>/dlls/liblz4.dll
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








