# Generated from jar.pro.

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
    src/org/qtproject/qt/android/QtServiceDelegate.java # special case
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}Android # special case
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

install_jar(Qt${QtBase_VERSION_MAJOR}Android # special case
    DESTINATION jar
    COMPONENT Devel
)

