# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(UicTest LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

set(CMAKE_AUTOMOC ON)

if (NOT DO_NOT_GENERATE_FILE)
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mainwindow.cpp"
       CONTENT " \
#include \"${CMAKE_CURRENT_SOURCE_DIR}/../UicBuildFolderLeakageCommon/mainwindow.h\" \n \
#include \"${MAINWINDOW_UI_PATH}ui_mainwindow.h\" \n \
MainWindow::MainWindow(QWidget* parent) \n \
  : QMainWindow(parent) \n \
  , ui(new Ui::MainWindow) \n \
{ \n \
  ui->setupUi(this); \n \
} \n \
 \n \
MainWindow::~MainWindow() \n \
{ \n \
  delete ui; \n \
} \n \
")
endif()

qt_add_executable(example
    ../UicBuildFolderLeakageCommon/main.cpp
    ../UicBuildFolderLeakageCommon/mainwindow.h
    mainwindow.ui
)

if (${DO_NOT_GENERATE_FILE})
  target_sources(example PRIVATE mainwindow.cpp)
else()
  target_sources(example PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/mainwindow.cpp")
endif()

target_link_libraries(example PRIVATE Qt6::Widgets
                                      Qt6::Core
                                      Qt6::Gui)

if (NOT UI_NO_CONFIG_OPTIONS)
  set(uic_options "$<$<CONFIG:Debug>:-a>")
endif()
qt6_add_ui(example
    INCLUDE_PREFIX "${MAINWINDOW_UI_PATH}"
    SOURCES "mainwindow.ui"
    OPTIONS "${uic_options}")

if(ADD_NEW_UI)
    qt6_add_ui(example INCLUDE_PREFIX "${NEW_UI_PATH}"
        SOURCES "subdir/mainwindow.ui"
        OPTIONS "${uic_options}")
endif()

# Enable AUTOUIC after qt6_add_ui() has been called
if (CMAKE_AUTOUIC)
    set_property(TARGET example PROPERTY AUTOUIC ON)
endif()


if (TEST_WITH_QRC)
    qt_add_resources(example "example.qrc" FILES "mainwindow.ui")
endif()
