# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#! [qt_wrap_cpp]
set(SOURCES myapp.cpp main.cpp)
qt_wrap_cpp(SOURCES myapp.h)
qt_add_executable(myapp ${SOURCES})
#! [qt_wrap_cpp]

#! [qt_add_resources]
set(SOURCES main.cpp)
qt_add_resources(SOURCES example.qrc)
qt_add_executable(myapp ${SOURCES})
#! [qt_add_resources]

#! [qt_add_resources_target]
qt_add_executable(myapp main.cpp)
qt_add_resources(myapp "images"
    PREFIX "/images"
    FILES image1.png image2.png)
#! [qt_add_resources_target]

#! [qt_add_big_resources]
set(SOURCES main.cpp)
qt_add_big_resources(SOURCES big_resource.qrc)

# Have big_resource.qrc treated as a source file by Qt Creator
list(APPEND SOURCES big_resource.qrc)
set_property(SOURCE big_resource.qrc PROPERTY SKIP_AUTORCC ON)

qt_add_executable(myapp ${SOURCES})
#! [qt_add_big_resources]

#! [qt_add_binary_resources]
qt_add_binary_resources(resources project.qrc OPTIONS -no-compress)
add_dependencies(myapp resources)
#! [qt_add_binary_resources]

#! [qt_generate_moc]
qt_generate_moc(main.cpp main.moc TARGET myapp)
#! [qt_generate_moc]

#! [qt_import_plugins]
qt_add_executable(myapp main.cpp)
target_link_libraries(myapp Qt::Gui Qt::Sql)
qt_import_plugins(myapp
    INCLUDE Qt::QCocoaIntegrationPlugin
    EXCLUDE Qt::QMinimalIntegrationPlugin
    INCLUDE_BY_TYPE imageformats Qt::QGifPlugin Qt::QJpegPlugin
    EXCLUDE_BY_TYPE sqldrivers
)
#! [qt_import_plugins]

#! [qt_add_executable_simple]
qt_add_executable(simpleapp main.cpp)
#! [qt_add_executable_simple]

#! [qt_add_executable_deferred]
qt_add_executable(complexapp MANUAL_FINALIZATION complex.cpp)
set_target_properties(complexapp PROPERTIES OUTPUT_NAME Complexify)
qt_finalize_target(complexapp)
#! [qt_add_executable_deferred]

#! [qt_android_deploy_basic]
qt_android_generate_deployment_settings(myapp)
qt_android_add_apk_target(myapp)
#! [qt_android_deploy_basic]

#! [qt_finalize_project_manual]
cmake_minimum_required(VERSIONS 3.16)

project(MyProject LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(MyApp main.cpp)
add_subdirectory(mylib)

qt_finalize_project()
#! [qt_finalize_project_manual]
