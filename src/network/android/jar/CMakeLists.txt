# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt/android/network/QtNetwork.java
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetwork # special case
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetwork # special case
    DESTINATION ${destination}
    COMPONENT Devel
)

