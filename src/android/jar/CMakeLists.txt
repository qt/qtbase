# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(java_sources
    src/org/qtproject/qt/android/QtAccessibilityDelegate.java
    src/org/qtproject/qt/android/QtNativeAccessibility.java
    src/org/qtproject/qt/android/CursorHandle.java
    src/org/qtproject/qt/android/EditContextView.java
    src/org/qtproject/qt/android/EditPopupMenu.java
    src/org/qtproject/qt/android/ExtractStyle.java
    src/org/qtproject/qt/android/QtApplicationBase.java
    src/org/qtproject/qt/android/QtActivityBase.java
    src/org/qtproject/qt/android/QtServiceBase.java
    src/org/qtproject/qt/android/QtActivityDelegate.java
    src/org/qtproject/qt/android/QtInputDelegate.java
    src/org/qtproject/qt/android/QtLoader.java
    src/org/qtproject/qt/android/QtActivityLoader.java
    src/org/qtproject/qt/android/QtServiceLoader.java
    src/org/qtproject/qt/android/QtEditText.java
    src/org/qtproject/qt/android/QtInputConnection.java
    src/org/qtproject/qt/android/QtLayout.java
    src/org/qtproject/qt/android/QtMessageDialogHelper.java
    src/org/qtproject/qt/android/QtNative.java
    src/org/qtproject/qt/android/QtSurfaceInterface.java
    src/org/qtproject/qt/android/QtSurface.java
    src/org/qtproject/qt/android/QtTextureView.java
    src/org/qtproject/qt/android/QtThread.java
    src/org/qtproject/qt/android/extras/QtAndroidBinder.java
    src/org/qtproject/qt/android/extras/QtAndroidServiceConnection.java
    src/org/qtproject/qt/android/extras/QtNative.java
    src/org/qtproject/qt/android/QtClipboardManager.java
    src/org/qtproject/qt/android/QtDisplayManager.java
    src/org/qtproject/qt/android/UsedFromNativeCode.java
    src/org/qtproject/qt/android/QtRootLayout.java
    src/org/qtproject/qt/android/QtWindow.java
    src/org/qtproject/qt/android/QtActivityDelegateBase.java
    src/org/qtproject/qt/android/QtEmbeddedDelegate.java
    src/org/qtproject/qt/android/QtEmbeddedViewInterfaceFactory.java
    src/org/qtproject/qt/android/QtEmbeddedLoader.java
    src/org/qtproject/qt/android/QtView.java
    src/org/qtproject/qt/android/QtEmbeddedViewInterface.java
    src/org/qtproject/qt/android/QtServiceEmbeddedDelegate.java
    src/org/qtproject/qt/android/BackendRegister.java
    src/org/qtproject/qt/android/QtWindowInterface.java
    src/org/qtproject/qt/android/QtAccessibilityInterface.java
    src/org/qtproject/qt/android/QtMenuInterface.java
    src/org/qtproject/qt/android/QtInputInterface.java
    src/org/qtproject/qt/android/QtAbstractItemModel.java
    src/org/qtproject/qt/android/QtAbstractItemModelProxy.java
    src/org/qtproject/qt/android/QtModelIndex.java
    src/org/qtproject/qt/android/QtAbstractListModel.java
    src/org/qtproject/qt/android/QtSignalListener.java
    src/org/qtproject/qt/android/BackgroundActionsTracker.java
    src/org/qtproject/qt/android/QtApkFileEngine.java
    src/org/qtproject/qt/android/QtContentFileEngine.java
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
