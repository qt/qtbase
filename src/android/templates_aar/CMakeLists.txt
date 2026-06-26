# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Android aar specific template files

set(templates_aar_files
    "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml")

add_custom_target(Qt${QtBase_VERSION_MAJOR}AndroidAarTemplates
    SOURCES
        ${templates_aar_files}
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/android/templates_aar")

qt_copy_or_install(FILES ${templates_aar_files}
                   DESTINATION "${destination}")

if(NOT QT_WILL_INSTALL)
    qt_internal_copy_at_build_time(TARGET Qt${QtBase_VERSION_MAJOR}AndroidAarTemplates
        FILES ${templates_aar_files}
        DESTINATION ${destination}
    )
endif()
