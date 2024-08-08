# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(mochelpers)

# QTBUG-88538
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(qmakelib)
    if(QT_FEATURE_qmake)
        add_subdirectory(qmake)
    endif()
    add_subdirectory(moc)
    add_subdirectory(rcc)
    add_subdirectory(qt_cmake_create)
endif()
# QTBUG-88538
if(TARGET Qt::Widgets AND NOT ANDROID AND NOT IOS)
    add_subdirectory(uic)
endif()
if(run_dbus_tests)
    add_subdirectory(qdbuscpp2xml)
    add_subdirectory(qdbusxml2cpp)
endif()
if(TARGET Qt::Gui AND QT_FEATURE_process AND NOT CMAKE_CROSSCOMPILING)
    # testapp (windeployqt) and source_basicapp (macdeployqt) require QtGui.
    if(QT_FEATURE_macdeployqt)
        add_subdirectory(macdeployqt)
    endif()
    if(QT_FEATURE_windeployqt AND BUILD_SHARED_LIBS)
        # windeployqt does not work with static Qt builds. See QTBUG-69427.
        add_subdirectory(windeployqt)
    endif()
endif()
