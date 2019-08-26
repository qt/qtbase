# special case begin
# This is a manually edited file

set(path_prefix ${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt5/android/bearer)

set(java_sources
    ${path_prefix}/QtNetworkReceiver.java
)

add_jar(QtAndroidBearer
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    )

install_jar(QtAndroidBearer
    DESTINATION jar
    COMPONENT Devel)

# special case end
