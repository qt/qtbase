# QtAndroid Jar

# special case begin
set(path_prefix ${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt5/android/)

set(java_sources
    ${path_prefix}/accessibility/QtAccessibilityDelegate.java
    ${path_prefix}/accessibility/QtNativeAccessibility.java
    ${path_prefix}/CursorHandle.java
    ${path_prefix}/EditContextView.java
    ${path_prefix}/EditPopupMenu.java
    ${path_prefix}/ExtractStyle.java
    ${path_prefix}/QtActivityDelegate.java
    ${path_prefix}/QtEditText.java
    ${path_prefix}/QtInputConnection.java
    ${path_prefix}/QtLayout.java
    ${path_prefix}/QtMessageDialogHelper.java
    ${path_prefix}/QtNative.java
    ${path_prefix}/QtNativeLibrariesDir.java
    ${path_prefix}/QtServiceDelegate.java
    ${path_prefix}/QtSurface.java
    ${path_prefix}/QtThread.java)

add_jar(QtAndroid
    INCLUDE_JARS ${android_jar}
    SOURCES ${java_sources}
    )

install_jar(QtAndroid
    DESTINATION jar
    COMPONENT Devel)
# special case end
