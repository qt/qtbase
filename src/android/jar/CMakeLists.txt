# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(java_sources
    src/org/qtproject/qt/android/accessibility/QtAccessibilityDelegate.java
    src/org/qtproject/qt/android/accessibility/QtNativeAccessibility.java
    src/org/qtproject/qt/android/CursorHandle.java
    src/org/qtproject/qt/android/EditContextView.java
    src/org/qtproject/qt/android/EditPopupMenu.java
    src/org/qtproject/qt/android/ExtractStyle.java
    src/org/qtproject/qt/android/QtActivityDelegate.java
    src/org/qtproject/qt/android/QtEditText.java
    src/org/qtproject/qt/android/QtInputConnection.java
    src/org/qtproject/qt/android/QtLayout.java
    src/org/qtproject/qt/android/QtMessageDialogHelper.java
    src/org/qtproject/qt/android/QtNative.java
    src/org/qtproject/qt/android/QtNativeLibrariesDir.java
    src/org/qtproject/qt/android/QtSurface.java
    src/org/qtproject/qt/android/QtThread.java
    src/org/qtproject/qt/android/QtServiceDelegate.java
    src/org/qtproject/qt/android/extras//QtAndroidBinder.java
    src/org/qtproject/qt/android/extras/QtAndroidServiceConnection.java
    src/org/qtproject/qt/android/extras/QtNative.java
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}Android
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtBase_VERSION_MAJOR}Android
    DESTINATION ${destination}
    COMPONENT Devel
)
