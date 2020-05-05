cmake_minimum_required(VERSION 3.1)

project(versionless_targets)

set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

find_package(Qt5Core REQUIRED)

if (NOT TARGET Qt5::Core)
    message(SEND_ERROR "Qt5::Core target not defined!")
endif()

if (TARGET Qt::Core)
    message(SEND_ERROR "Qt::Core target defined despite QT_NO_CREATE_VERSIONLESS_TARGETS!")
endif()

set(QT_NO_CREATE_VERSIONLESS_TARGETS OFF)

find_package(Qt5Core REQUIRED)

if (NOT TARGET Qt::Core)
    message(SEND_ERROR "Qt::Core target not defined!")
endif()
