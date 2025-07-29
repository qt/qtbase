# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#! [qt_wrap_cpp_1]
set(SOURCES myapp.cpp main.cpp)
qt_wrap_cpp(SOURCES myapp.h)
qt_add_executable(myapp ${SOURCES})
#! [qt_wrap_cpp_1]

#! [qt_wrap_cpp_2]
set(SOURCES myapp.cpp main.cpp)
qt_wrap_cpp(SOURCES myapp.h
            TARGET myapp
            OPTIONS
            "$<$<CONFIG:Debug>:-DMY_OPTION_FOR_DEBUG>"
            "-DDEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)"
            "$<$<CONFIG:Debug>:-DDEFINE_CMDLINE_SIGNAL_IN_GENEX=void cmdlineSignal(const QMap<int$<COMMA> int$<ANGLE-R> &i)>")
qt_add_executable(myapp ${SOURCES})
#! [qt_wrap_cpp_2]

#! [qt_wrap_cpp_3]
set(SOURCES myapp.cpp main.cpp)
qt_wrap_cpp(SOURCES myapp.h
            TARGET myapp)
qt_add_executable(myapp ${SOURCES})
target_compile_definitions(myapp PRIVATE "$<$<CONFIG:Debug>:MY_OPTION_FOR_DEBUG>"
                                         "DEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)"
                                         "$<$<BOOL:TRUE>:DEFINE_CMDLINE_SIGNAL_IN_GENEX=void cmdlineSignal(const QMap<int$<COMMA> int$<ANGLE-R> &i)>")
#! [qt_wrap_cpp_3]

#! [qt_wrap_cpp_4]
qt_add_executable(myapp myapp.cpp main.cpp)
qt_wrap_cpp(myapp myapp.cpp)
#! [qt_wrap_cpp_4]

#! [qt_add_resources]
set(sources main.cpp)
qt_add_resources(sources example.qrc)
qt_add_executable(myapp ${sources})
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
target_link_libraries(myapp Qt6::Gui Qt6::Sql)
qt_import_plugins(myapp
    INCLUDE Qt6::QCocoaIntegrationPlugin
    EXCLUDE Qt6::QMinimalIntegrationPlugin
    INCLUDE_BY_TYPE imageformats Qt6::QGifPlugin Qt6::QJpegPlugin
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

#! [qt_add_android_permission]
qt_add_executable(myapp
    // ...
)
qt_add_android_permission(myapp
    NAME android.permission.BLUETOOTH_SCAN
    ATTRIBUTES
        minSdkVersion 31
        usesPermissionFlags neverForLocation
)
qt_add_android_permission(myapp
    NAME android.permission.ACCESS_COARSE_LOCATION
)
#! [qt_add_android_permission]

#! [qt_finalize_project_manual]
cmake_minimum_required(VERSIONS 3.16)

project(MyProject LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(MyApp main.cpp)
add_subdirectory(mylib)

qt_finalize_project()
#! [qt_finalize_project_manual]

#! [AUTOGEN_BETTER_GRAPH_MULTI_CONFIG_1]
set(CMAKE_AUTOGEN_BETTER_GRAPH_MULTI_CONFIG ON)
#! [AUTOGEN_BETTER_GRAPH_MULTI_CONFIG_1]

#! [AUTOGEN_BETTER_GRAPH_MULTI_CONFIG_2]
set_target_properties(app PROPERTIES AUTOGEN_BETTER_GRAPH_MULTI_CONFIG ON)
#! [AUTOGEN_BETTER_GRAPH_MULTI_CONFIG_2]
