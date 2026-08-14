# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# HarmonyOS template files

set(template_files
    "${CMAKE_CURRENT_SOURCE_DIR}/build-profile.json5"
    "${CMAKE_CURRENT_SOURCE_DIR}/hvigorfile.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/oh-package.json5"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
)

set(template_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/AppScope"
    "${CMAKE_CURRENT_SOURCE_DIR}/entry"
    "${CMAKE_CURRENT_SOURCE_DIR}/hvigor"
)

add_custom_target(Qt${QtBase_VERSION_MAJOR}HarmonyOSTemplates
    SOURCES
        ${template_files}
)

qt_path_join(build_dir_destination
    ${QtBase_BINARY_DIR} ${INSTALL_DATADIR} "src/harmonyos/templates")

file(COPY ${template_files} ${template_directories}
    DESTINATION "${build_dir_destination}")

qt_internal_copy_at_build_time(TARGET Qt${QtBase_VERSION_MAJOR}HarmonyOSTemplates
    FILES ${template_files}
    DIRECTORIES ${template_directories}
    DESTINATION ${build_dir_destination}
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/harmonyos/templates")
qt_install(FILES ${template_files} DESTINATION "${destination}")
qt_install(DIRECTORY ${template_directories} DESTINATION "${destination}")
