# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


set(java_sources
    jar/src/org/qtproject/qt/android/networkinformation/QtAndroidNetworkInformation.java
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
    GRADLE_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jar
)

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend
    DESTINATION ${destination}
    COMPONENT Devel
)

qt_internal_add_plugin(QAndroidNetworkInformationPlugin
    OUTPUT_NAME qandroidnetworkinformation
    CLASS_NAME QAndroidNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF ANDROID
    SOURCES
        qandroidnetworkinformationbackend.cpp
        wrapper/androidconnectivitymanager.cpp wrapper/androidconnectivitymanager.h
    LIBRARIES
        Qt::NetworkPrivate
    DEFINES
        "QT_ENABLE_STRICT_MODE_UP_TO=QT_VERSION_CHECK(6,11,0)"
        QT_NO_CAST_FROM_ASCII
)

set_property(
    TARGET
        QAndroidNetworkInformationPlugin
    APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtBase_VERSION_MAJOR}AndroidNetworkInformationBackend.jar
)

qt_internal_add_android_permission(QAndroidNetworkInformationPlugin
    NAME android.permission.ACCESS_NETWORK_STATE
)
