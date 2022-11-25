# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Android template files

# special case skip regeneration
# special case begin
set(template_files
    "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/build.gradle")

set(template_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/res")

add_custom_target(Qt${QtBase_VERSION_MAJOR}AndroidTemplates
    SOURCES
        ${template_files}
        "${CMAKE_CURRENT_SOURCE_DIR}/res/values/libs.xml"
)

qt_path_join(destination ${QT_INSTALL_DIR} "src/android/templates")

qt_copy_or_install(FILES ${template_files}
                   DESTINATION "${destination}")

qt_copy_or_install(DIRECTORY ${template_directories}
                   DESTINATION "${destination}")
# special case end
