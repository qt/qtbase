# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt5/android/bearer/QtNetworkReceiver.java
)

add_jar(QtAndroidBearer
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
)

install_jar(QtAndroidBearer
    DESTINATION jar
    COMPONENT Devel
)

