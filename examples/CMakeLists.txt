# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(QT_BUILD_STANDALONE_EXAMPLES)
    # Needed for early feature values, despite it being found later on in qt_build_tests().
    # Needs to be find_package, not qt_find_package, because qt_find_package doesn't handle finding
    # component for a super-package that has already been found.
    find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Core)

    # Modify the optimization flags specifically for qtbase standalone examples. Other projects
    # are handled by qt_enable_cmake_languages().
    qt_internal_set_up_config_optimizations_like_in_qmake()
endif()

qt_examples_build_begin(EXTERNAL_BUILD)

add_compile_definitions(QT_NO_CONTEXTLESS_CONNECT)

add_subdirectory(corelib)
if(TARGET Qt6::DBus)
    add_subdirectory(dbus)
endif()
if(TARGET Qt6::Network)
    add_subdirectory(network)
endif()
if(TARGET Qt6::Test)
    add_subdirectory(qtestlib)
endif()
if(TARGET Qt6::Concurrent)
    add_subdirectory(qtconcurrent)
endif()
if(TARGET Qt6::Sql)
    add_subdirectory(sql)
endif()
if(TARGET Qt6::Widgets)
    add_subdirectory(widgets)
endif()
if(TARGET Qt6::Xml)
    add_subdirectory(xml)
endif()
if(TARGET Qt6::Gui)
    add_subdirectory(gui)
endif()
if(QT_FEATURE_opengl AND TARGET Qt6::Gui)
    add_subdirectory(opengl)
endif()
if(QT_FEATURE_vulkan AND TARGET Qt6::Gui)
    add_subdirectory(vulkan)
endif()

qt_examples_build_end()
