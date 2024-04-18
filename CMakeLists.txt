cmake_minimum_required(VERSION 3.14)

project(cutesdeck VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 NAMES Qt6 REQUIRED COMPONENTS Core Gui Concurrent Qml)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBUDEV libudev)

qt6_policy(SET QTP0001 NEW)
qt_standard_project_setup(REQUIRES 6.5)

add_library(cutesdeck SHARED  
  cutesdeck.cpp
  cutesdeck.h  
)

ADD_CUSTOM_TARGET(examples SOURCES example.qml)

set(CMAKE_INSTALL_PREFIX ${CMAKE_PREFIX_PATH})
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

set_target_properties(cutesdeck PROPERTIES PUBLIC_HEADER cutesdeck.h)

target_include_directories(cutesdeck SYSTEM PUBLIC ${LIBUDEV_INCLUDE_DIRS})
target_link_libraries(cutesdeck PRIVATE Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Qml ${LIBUDEV_LIBRARIES})

qt_add_qml_module(cutesdeck
    URI "CutesDeck"
    PLUGIN_TARGET cutesdeckplugin
    DEPENDENCIES QtQuick
    OUTPUT_DIRECTORY CutesDeck
)

include(GNUInstallDirs)
#include(${QT_DEPLOY_SUPPORT})

#qt_generate_deploy_app_script(
#    TARGET cutesdeckplugin
#    FILENAME_VARIABLE deploy_script
#    NO_UNSUPPORTED_PLATFORM_ERROR
#)
#install(SCRIPT ${deploy_script})

install(TARGETS cutesdeck
    BUNDLE DESTINATION .
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/CutesDeck/ DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/CutesDeck/)

set_target_properties(cutesdeckplugin PROPERTIES INSTALL_RPATH "$ORIGIN/../../lib")
install(TARGETS cutesdeckplugin LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/CutesDeck/)
