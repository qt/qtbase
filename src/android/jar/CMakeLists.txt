# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt5/android/accessibility/QtAccessibilityDelegate.java
    src/org/qtproject/qt5/android/accessibility/QtNativeAccessibility.java
    src/org/qtproject/qt5/android/CursorHandle.java
    src/org/qtproject/qt5/android/EditContextView.java
    src/org/qtproject/qt5/android/EditPopupMenu.java
    src/org/qtproject/qt5/android/ExtractStyle.java
    src/org/qtproject/qt5/android/QtActivityDelegate.java
    src/org/qtproject/qt5/android/QtEditText.java
    src/org/qtproject/qt5/android/QtInputConnection.java
    src/org/qtproject/qt5/android/QtLayout.java
    src/org/qtproject/qt5/android/QtMessageDialogHelper.java
    src/org/qtproject/qt5/android/QtNative.java
    src/org/qtproject/qt5/android/QtNativeLibrariesDir.java
    src/org/qtproject/qt5/android/QtSurface.java
    src/org/qtproject/qt5/android/QtThread.java
    src/org/qtproject/qt5/android/QtServiceDelegate.java # special case
)

add_jar(QtAndroid
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
)

install_jar(QtAndroid
    DESTINATION jar
    COMPONENT Devel
)

