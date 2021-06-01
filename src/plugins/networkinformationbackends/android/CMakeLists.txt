
set(java_sources
    jar/src/org/qtproject/qt/android/networkinformation/QtAndroidNetworkInformation.java
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

install_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend
    DESTINATION jar
    COMPONENT Devel
)

qt_internal_add_plugin(QAndroidNetworkInformationBackendPlugin
    OUTPUT_NAME androidnetworkinformationbackend
    CLASS_NAME QAndroidNetworkInformationBackendFactory
    TYPE networkinformationbackends
    DEFAULT_IF ANDROID
    SOURCES
        qandroidnetworkinformationbackend.cpp
        wrapper/androidconnectivitymanager.cpp wrapper/androidconnectivitymanager.h
    LIBRARIES
        Qt::NetworkPrivate
    DEFINES
        QT_USE_QSTRINGBUILDER
)

set_property(
    TARGET
        QAndroidNetworkInformationBackendPlugin
    APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend.jar
)

set_property(
    TARGET
        QAndroidNetworkInformationBackendPlugin
    APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.ACCESS_NETWORK_STATE
)
