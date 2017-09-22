cmake_minimum_required (VERSION 3.9 FATAL_ERROR)
project(uibase VERSION 0.1 LANGUAGES CXX)
set(library_name "uibase")

## Files ##
# Main
SET(HDRS
    include/${library_name}/iplugin.h
    include/${library_name}/iplugintool.h
    include/${library_name}/iplugininstaller.h
    include/${library_name}/iplugininstallersimple.h
    include/${library_name}/iplugininstallercustom.h
    include/${library_name}/iplugindiagnose.h
    include/${library_name}/ipluginproxy.h
    include/${library_name}/ipluginlist.h
    include/${library_name}/ipluginmodpage.h
    include/${library_name}/ipluginpreview.h
    include/${library_name}/iplugingame.h
    include/${library_name}/ipluginfilemapper.h
    include/${library_name}/utility.h
    include/${library_name}/textviewer.h
    include/${library_name}/finddialog.h
    include/${library_name}/report.h
    include/${library_name}/versioninfo.h
    include/${library_name}/imoinfo.h
    include/${library_name}/imodinterface.h
    include/${library_name}/lineeditclear.h
    include/${library_name}/dllimport.h
    include/${library_name}/directorytree.h
    include/${library_name}/mytree.h
    include/${library_name}/installationtester.h
    include/${library_name}/tutorialmanager.h
    include/${library_name}/tutorialcontrol.h
    include/${library_name}/tutorabledialog.h
    include/${library_name}/iinstallationmanager.h
    include/${library_name}/scopeguard.h
    include/${library_name}/pluginsetting.h
    include/${library_name}/diagnosisreport.h
    include/${library_name}/guessedvalue.h
    include/${library_name}/json.h
    include/${library_name}/imodrepositorybridge.h
    include/${library_name}/idownloadmanager.h
    include/${library_name}/nxmurl.h
    include/${library_name}/taskprogressmanager.h
    include/${library_name}/questionboxmemory.h
    include/${library_name}/sortabletreewidget.h
    include/${library_name}/imodlist.h
    include/${library_name}/modrepositoryfileinfo.h
    include/${library_name}/executableinfo.h
    include/${library_name}/iprofile.h
    include/${library_name}/delayedfilewriter.h
    include/${library_name}/filenamestring.h
    include/${library_name}/filemapping.h
    include/${library_name}/safewritefile.h
)

SET(SRCS
    src/utility.cpp
    src/textviewer.cpp
    src/finddialog.cpp
    src/report.cpp
    src/versioninfo.cpp
    src/lineeditclear.cpp
    src/mytree.cpp
    src/installationtester.cpp
    src/tutorialmanager.cpp
    src/tutorialcontrol.cpp
    src/tutorabledialog.cpp
    src/scopeguard.cpp
    src/pluginsetting.cpp
    src/diagnosisreport.cpp
    src/directorytree.cpp
    src/iplugininstaller.cpp
    src/guessedvalue.cpp
    src/json.cpp
    src/imodrepositorybridge.cpp
    src/nxmurl.cpp
    src/taskprogressmanager.cpp
    src/questionboxmemory.cpp
    src/modrepositoryfileinfo.cpp
    src/sortabletreewidget.cpp
    src/executableinfo.cpp
    src/delayedfilewriter.cpp
    src/filenamestring.cpp
    src/safewritefile.cpp
)

SET(QTs
    src/finddialog.ui
    src/questionboxmemory.ui
    src/textviewer.ui
)
source_group(QT FILES ${QTs})

## Dependencies ##
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WinExtras REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(Qt5Quick REQUIRED)

## Library ##
add_library(${library_name} SHARED ${HDRS} ${SRCS})
add_definitions(-DUIBASE_EXPORT)
# Headers
target_include_directories(${library_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src
)
# Dependencies
target_link_libraries(${library_name} PUBLIC
    common
    Qt5::Widgets Qt5::Script Qt5::Quick Qt5::WinExtras Boost::boost
)
set_target_properties(${library_name} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
# Install Targets
install(TARGETS ${library_name} EXPORT ${library_name}Config
    ARCHIVE  DESTINATION lib # <prefix>/lib. DLL import libs.
    LIBRARY  DESTINATION lib # <prefix>/lib. Shared Libs, non DLL.
    RUNTIME  DESTINATION bin # <prefix>/bin. EXEs, DLLs and shit.
)
INSTALL(FILES $<TARGET_FILE_DIR:${library_name}>/uibase.pdb DESTINATION pdb)
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








