#####################################################################
## QtAndroidBearer Binary:
#####################################################################

set(java_sources
    src/org/qtproject/qt5/android/bearer/QtNetworkReceiver.java
)

add_jar(QtAndroidBearer
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    )

install_jar(QtAndroidBearer
    DESTINATION jar
    COMPONENT Devel)

#### Keys ignored in scope 1:.:.:jar.pro:<TRUE>:
# JAVACLASSPATH = "$$PWD/src"
# JAVASOURCES = "$$PWD/src/org/qtproject/qt5/android/bearer/QtNetworkReceiver.java"
# _LOADED = "qt_build_paths"

