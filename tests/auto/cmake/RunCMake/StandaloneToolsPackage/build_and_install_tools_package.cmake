# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Core BuildInternals)

# Avoid erorrs in CI about unsupported SDK and Xcode versions on older CI macOS versions.
if(APPLE)
    set(QT_NO_APPLE_SDK_AND_XCODE_CHECK ON)
endif()

qt_internal_project_setup()

qt_build_repo_begin()

set(base_name "Garage")
set(extra_file_base_path "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}${base_name}")

# Add a C++ tool that should be part of the standalone tools package.
qt_get_tool_target_name(pliers_target pliers)
qt_internal_add_tool(${pliers_target}
    SOURCES
        pliers.cpp
)
qt_install(TARGETS ${pliers_target}
    EXPORT "${INSTALL_CMAKE_NAMESPACE}GarageToolsTargets"
    RUNTIME DESTINATION "${QT_INSTALL_BINDIR}"
)

# Add a standalone tools package.
qt_internal_add_tools_package(
    PACKAGE_BASE_NAME ${base_name}
    EXTRA_CMAKE_FILES
        "${extra_file_base_path}ToolsExtraInclude.cmake"
    EXTRA_CMAKE_INCLUDES
        "${QT_CMAKE_EXPORT_NAMESPACE}${base_name}ToolsExtraInclude.cmake"
    TOOL_TARGETS
        ${pliers_target}
)

# Check that we can add third party dependencies to the tools package.
qt_internal_record_tools_package_extra_third_party_dependency(
    PACKAGE_BASE_NAME ${base_name}
    DEPENDENCY_PACKAGE_NAME WrapScrewdriver)

# Add a module that should be look up the standalone tools package when the module itself is
# looked up.
qt_internal_add_module(Workshop
    HEADER_MODULE
    NO_MODULE_HEADERS
    NO_PRIVATE_MODULE
    NO_GENERATE_CPP_EXPORTS
    NO_ADDITIONAL_TARGET_INFO
    NO_GENERATE_METATYPES
    NO_PACKAGE_CONFIG_FILE
    NO_MODULE_JSON_FILE
    NO_QMAKE_SUPPORT_FILES
)
qt_record_extra_qt_main_tools_package_dependency(Workshop GarageTools "6")

qt_build_repo_post_process()
qt_build_repo_end()
