# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(templates_files
    "${CMAKE_CURRENT_SOURCE_DIR}/build.gradle"
    "${CMAKE_CURRENT_SOURCE_DIR}/settings.gradle.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle.properties"
)

set(gradle_template_files
    "${CMAKE_CURRENT_SOURCE_DIR}/gradle/libs.versions.toml"
)

set(app_template_files
    "${CMAKE_CURRENT_SOURCE_DIR}/app/build.gradle.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/AndroidManifest.xml.in"
)

set(dynamic_feature_files
    "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_feature/AndroidManifest.xml.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_feature/build.gradle.in"
)

add_custom_target(Qt6AndroidCMakeTemplates
    SOURCES
        ${templates_files}
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/android/templates_cmake")

qt_copy_or_install(FILES ${templates_files} DESTINATION "${destination}")
qt_copy_or_install(FILES ${gradle_template_files} DESTINATION "${destination}/gradle")
qt_copy_or_install(FILES ${app_template_files} DESTINATION "${destination}/app")
qt_copy_or_install(FILES ${dynamic_feature_files} DESTINATION "${destination}/dynamic_feature")

if(NOT QT_WILL_INSTALL)
    qt_internal_copy_at_build_time(TARGET Qt6AndroidCMakeTemplates
        FILES ${templates_files}
        DESTINATION ${destination}
    )

    qt_internal_copy_at_build_time(TARGET Qt6AndroidCMakeTemplates
        FILES ${gradle_template_files}
        DESTINATION ${destination}/gradle
    )

    qt_internal_copy_at_build_time(TARGET Qt6AndroidCMakeTemplates
        FILES ${app_templates_files}
        DESTINATION ${destination}/app
    )

    qt_internal_copy_at_build_time(TARGET Qt6AndroidCMakeTemplates
        FILES ${dynamic_feature_files}
        DESTINATION ${destination}/dynamic_feature
    )
endif()
