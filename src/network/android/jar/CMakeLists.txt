# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt5/android/network/QtNetwork.java
)

add_jar(QtAndroidNetwork
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
)

install_jar(QtAndroidNetwork
    DESTINATION jar
    COMPONENT Devel
)

