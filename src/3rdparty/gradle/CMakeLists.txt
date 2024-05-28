# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
set(gradle_programs
    "${CMAKE_CURRENT_SOURCE_DIR}/gradlew"
)
set(gradle_files
    "${CMAKE_CURRENT_SOURCE_DIR}/gradlew.bat"
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle.properties"
)
set(gradle_wrapper
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle"
)
set(gradle_wrapper_files
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle/wrapper/gradle-wrapper.properties"
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle/wrapper/gradle-wrapper.jar"
)

add_custom_target(Qt${QtBase_VERSION_MAJOR}GradleScripts
    SOURCES
        ${gradle_programs}
        ${gradle_files}
        ${gradle_wrapper_files}
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/3rdparty/gradle")

qt_copy_or_install(
    PROGRAMS
        ${gradle_programs}
    DESTINATION
        "${destination}"
)

qt_copy_or_install(
    FILES
        ${gradle_files}
    DESTINATION
        "${destination}"
)

qt_copy_or_install(
    DIRECTORY
        ${gradle_wrapper}
    DESTINATION
        "${destination}"
)

if(NOT QT_WILL_INSTALL)
    qt_internal_copy_at_build_time(TARGET Qt${QtBase_VERSION_MAJOR}GradleScripts
        FILES ${gradle_programs} ${gradle_files}
        DIRECTORIES ${gradle_wrapper}
        DESTINATION ${destination}
    )
endif()
