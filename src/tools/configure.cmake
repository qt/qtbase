# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(LINUX OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(WaylandScanner MODULE PROVIDED_TARGETS Wayland::Scanner)
endif()

qt_feature("androiddeployqt" PRIVATE
    SECTION "Deployment"
    LABEL "Android deployment tool"
    PURPOSE "The Android deployment tool automates the process of creating Android packages."
    CONDITION NOT CMAKE_CROSSCOMPILING AND QT_FEATURE_regularexpression AND QT_FEATURE_settings)

qt_feature("macdeployqt" PRIVATE
    SECTION "Deployment"
    LABEL "macOS deployment tool"
    PURPOSE "The Mac deployment tool automates the process of creating a deployable application bundle that contains the Qt libraries as private frameworks."
    AUTODETECT CMAKE_HOST_APPLE
    CONDITION MACOS AND QT_FEATURE_thread)

qt_feature("windeployqt" PRIVATE
    SECTION "Deployment"
    LABEL "Windows deployment tool"
    PURPOSE "The Windows deployment tool is designed to automate the process of creating a deployable folder containing the Qt-related dependencies (libraries, QML imports, plugins, and translations) required to run the application from that folder. The folder can be easily bundled into an installation package."
    AUTODETECT CMAKE_HOST_WIN32
    CONDITION WIN32)

qt_feature("qmake" PRIVATE
    PURPOSE "The qmake tool helps simplify the build process for development projects across different platforms."
    CONDITION QT_FEATURE_settings AND QT_FEATURE_cborstreamwriter AND
        QT_FEATURE_datestring AND QT_FEATURE_regularexpression AND QT_FEATURE_temporaryfile)

qt_feature("qtwaylandscanner" PRIVATE
    CONDITION TARGET Wayland::Scanner
)

qt_configure_add_summary_section(NAME "Core tools")
qt_configure_add_summary_entry(ARGS "androiddeployqt")
qt_configure_add_summary_entry(ARGS "macdeployqt")
qt_configure_add_summary_entry(ARGS "windeployqt")
qt_configure_add_summary_entry(ARGS "qmake")
qt_configure_end_summary_section()

qt_configure_add_summary_section(NAME "Wayland tools")
qt_configure_add_summary_entry(ARGS "qtwaylandscanner")
qt_configure_end_summary_section()
