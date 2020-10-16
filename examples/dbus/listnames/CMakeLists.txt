# Generated from listnames.pro.

cmake_minimum_required(VERSION 3.14)
project(listnames LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/listnames")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS DBus)

qt_add_executable(listnames
    listnames.cpp
)
target_link_libraries(listnames PUBLIC
    Qt::Core
    Qt::DBus
)

install(TARGETS listnames
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
