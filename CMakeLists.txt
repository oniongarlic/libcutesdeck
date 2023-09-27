cmake_minimum_required(VERSION 3.14)

project(cutesdeck LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Concurrent Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Concurrent Qml)

find_package(PkgConfig REQUIRED)

pkg_check_modules (LIBUSB1 libusb-1.0)
pkg_check_modules (HIDAPI hidapi-libusb)
pkg_check_modules (EVDEV libevdev)

add_library(cutesdeck SHARED
  cutesdeck_global.h
  cutesdeck.cpp
  cutesdeck.h
)

set_target_properties(cutesdeck PROPERTIES PUBLIC_HEADER cutesdeck.h)
# set_target_properties(cutesdeck PROPERTIES PUBLIC_HEADER cutesdeck_global.h)

target_include_directories(cutesdeck SYSTEM PUBLIC ${LIBUSB1_INCLUDE_DIRS} ${HIDAPI_INCLUDE_DIRS} ${EVDEV_INCLUDE_DIRS})
target_link_libraries(cutesdeck PRIVATE Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Qml)
target_compile_definitions(cutesdeck PRIVATE CUTESDECK_LIBRARY ${LIBUSB1_LIBRARIES} ${HIDAPI_LIBRARIES} ${EVDEV_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS cutesdeck
    BUNDLE DESTINATION .
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
