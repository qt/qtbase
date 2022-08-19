# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The configure.cmake here does not get picked up automatically.
# Manually evaluate tool-related features.
include("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
qt_feature_evaluate_features("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")

add_subdirectory(uic)
if (QT_FEATURE_dbus)
    add_subdirectory(qdbuscpp2xml)
    add_subdirectory(qdbusxml2cpp)
endif()
add_subdirectory(qlalr)
add_subdirectory(qvkgen)
if (QT_FEATURE_commandlineparser)
    add_subdirectory(qtpaths)
endif()

if(QT_FEATURE_androiddeployqt)
    add_subdirectory(androiddeployqt)
    if(QT_FEATURE_gui AND QT_FEATURE_systemsemaphore)
        add_subdirectory(androidtestrunner)
    endif()
endif()

if(QT_FEATURE_macdeployqt)
    add_subdirectory(macdeployqt)
endif()

if(QT_FEATURE_windeployqt)
    add_subdirectory(windeployqt)
endif()
