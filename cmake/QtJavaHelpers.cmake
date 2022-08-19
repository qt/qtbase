# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function can be used to compile java sources into a jar package.

function(qt_internal_add_jar target)

    set(javac_target_version "${QT_ANDROID_JAVAC_TARGET}")
    if (NOT javac_target_version)
        set(javac_target_version "8")
    endif()

    set(javac_source_version "${QT_ANDROID_JAVAC_SOURCE}")
    if (NOT javac_source_version)
        set(javac_source_version "8")
    endif()

    set(CMAKE_JAVA_COMPILE_FLAGS -source "${javac_source_version}" -target "${javac_target_version}" -Xlint:unchecked -bootclasspath "${QT_ANDROID_JAR}")
    add_jar(${ARGV})

endfunction()
