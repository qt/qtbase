# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt/android/network/QtNetwork.java
)

qt_internal_add_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetwork # special case
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

install_jar(Qt${QtBase_VERSION_MAJOR}AndroidNetwork # special case
    DESTINATION jar
    COMPONENT Devel
)

