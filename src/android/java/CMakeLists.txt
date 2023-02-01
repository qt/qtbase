# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Android Java Resource files

set(resource_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(java_bindings
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtActivity.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtActivityLoader.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtService.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtServiceLoader.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtApplication.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/org/qtproject/qt/android/bindings/QtLoader.java"
)

set(strings_resouces
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-de/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-el/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-es/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-et/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-fa/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-fr/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-in/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-it/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-ja/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-ms/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-nb/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-nl/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-pl/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-pt-rBR/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-ro/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-ru/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-se/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-zh-rCN/strings.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/values-zh-rTW/strings.xml"
)

add_custom_target(Qt${QtBase_VERSION_MAJOR}AndroidBindings
    SOURCES
        ${java_bindings}
        ${strings_resouces}
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/android/java")

qt_copy_or_install(DIRECTORY ${resource_directories}
                   DESTINATION "${destination}"
)

if(NOT QT_WILL_INSTALL)
    qt_internal_copy_at_build_time(TARGET Qt${QtBase_VERSION_MAJOR}AndroidBindings
        DIRECTORIES ${resource_directories}
        DESTINATION ${destination})
endif()
