# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

function(create_sbom_lib_target target)
    add_library(${target} STATIC)
    target_sources(${target} PRIVATE sources/utils_helper.cpp)
    install(TARGETS ${target}
        EXPORT MainTargets
        ARCHIVE DESTINATION lib
    )
    _qt_internal_add_sbom(${target}
        TYPE "LIBRARY"
        RUNTIME_PATH bin
        ARCHIVE_PATH lib
        LIBRARY_PATH lib
    )

    # This is used the by spdx and cydx deps checking code.
    _qt_internal_sbom_get_spdx_id_for_target(${target} ${target}_spdx_id)
    set(${target}_spdx_id "${${target}_spdx_id}" PARENT_SCOPE)

    add_known_target(${target}
        CATEGORY "RELATIONSHIP"
        SPDX_ID "${${target}_spdx_id}"
    )
endfunction()

macro(set_common_sbom_begin_args out_var)
    set(${out_var}
        SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
        SUPPLIER "${SBOM_SUPPLIER}"
        SUPPLIER_URL "${SBOM_SUPPLIER_URL}"
        VERSION "${SBOM_VERSION}"
    )
endmacro()

set(SBOM_SUPPLIER "QtProjectTest")
set(SBOM_SUPPLIER_URL "https://qt-project.org/SbomTest")
set(SBOM_VERSION "1.0.0")

# Case 1, check that the default project system build tools are created
set(SBOM_PROJECT_NAME "001-auto-system-build-tools")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
)
sbom_test_record_project()

_qt_internal_sbom_get_project_default_system_build_tool_types(default_build_tools_types)
if(QT_GENERATE_SBOM)
    foreach(build_tool_type IN LISTS default_build_tools_types)
        _qt_internal_sbom_get_system_build_tool_target_for_type(
            BUILD_TOOL_TYPE "${build_tool_type}"
            OUT_VAR_TARGET target
        )
        if(NOT TARGET "${target}")
            message(FATAL_ERROR
                "Expected build tool target '${target}' for build tool type "
                "'${build_tool_type}' does not exist.")
        endif()
    endforeach()
endif()
_qt_internal_sbom_end_project()

# Case 2, check that the default project system build tools are not created
set(SBOM_PROJECT_NAME "002-no-auto-system-build-tools")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_ADD_BUILD_TOOLS
)
sbom_test_record_project()

if(QT_GENERATE_SBOM)
    foreach(build_tool_type IN LISTS default_build_tools_types)
        _qt_internal_sbom_get_system_build_tool_target_for_type(
            BUILD_TOOL_TYPE "${build_tool_type}"
            OUT_VAR_TARGET target
        )
        if(TARGET "${target}")
            message(FATAL_ERROR
                "Expected build tool target '${target}' for build tool type "
                "'${build_tool_type}' exists, when it shouldn't.")
        endif()
    endforeach()
endif()
_qt_internal_sbom_end_project()

# Case 3, check that we can create the pre-defined system tool types manually
set(SBOM_PROJECT_NAME "003-manual-predefined-system-build-tools")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_ADD_BUILD_TOOLS
)
sbom_test_record_project()

if(QT_GENERATE_SBOM)
    foreach(build_tool_type IN LISTS default_build_tools_types)
        _qt_internal_sbom_get_system_build_tool_target_for_type(
            BUILD_TOOL_TYPE "${build_tool_type}"
            OUT_VAR_TARGET target
        )
        _qt_internal_add_sbom_system_build_tool("${target}"
            BUILD_TOOL_TYPE "${build_tool_type}"
        )
        if(NOT TARGET "${target}")
            message(FATAL_ERROR
                "Expected build tool target '${target}' for build tool type "
                "'${build_tool_type}' does not exist.")
        endif()
    endforeach()
endif()
_qt_internal_sbom_end_project()

# Case 4, add a custom system build tool
set(SBOM_PROJECT_NAME "004-custom-system-build-tool")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_ADD_BUILD_TOOLS
)
sbom_test_record_project()

set(build_tool_target "BuildToolHammer")
_qt_internal_sbom_get_current_project_target(project_target)
_qt_internal_add_sbom_system_build_tool("${build_tool_target}"
    BUILD_TOOL_TYPE "CUSTOM"
    SBOM_ARGS
        FRIENDLY_PACKAGE_NAME "Hammer"
        PACKAGE_VERSION "1.2.3"
        PACKAGE_SUMMARY "Very shiny hammer"
        LICENSE_EXPRESSION "MIT"
        DOWNLOAD_LOCATION "https://veryshinyhammers.com/packages/hammer-1.2.3.tar.gz"
        SUPPLIER "Handyman Central"
        COPYRIGHTS "Handyman Central Copyright (C)"
        SBOM_RELATIONSHIP_ENTRIES
            SBOM_RELATIONSHIP_ENTRY
                SBOM_RELATIONSHIP_FROM
                    "${build_tool_target}"
                SBOM_RELATIONSHIP_TYPE
                    DEV_DEPENDENCY_OF
                SBOM_RELATIONSHIP_TO
                    "${project_target}"
                SBOM_RELATIONSHIP_COMMENT
                    "Project is hammered in using Hammer version 1.2.3"
)

if(QT_GENERATE_SBOM)
    if(NOT TARGET "${build_tool_target}")
        message(FATAL_ERROR
            "Expected build tool target '${build_tool_target}' does not exist.")
    endif()
endif()

# Case 4, add an extra relationship that a library is generated using the above system build tool
if(QT_GENERATE_SBOM)
    _qt_internal_sbom_get_spdx_id_for_target(${build_tool_target} ${build_tool_target}_spdx_id)
endif()
create_sbom_lib_target(Plank)
_qt_internal_extend_sbom(Plank
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "Plank"
            SBOM_RELATIONSHIP_TYPE
                GENERATED_FROM
            SBOM_RELATIONSHIP_TO
                "${build_tool_target}"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${Plank_spdx_id} GENERATED_FROM ${${build_tool_target}_spdx_id}")
add_cydx_v1_6_deps_to_result_file(Plank DEPS "${${build_tool_target}_spdx_id}")

_qt_internal_sbom_end_project()

# Case 5, add a (non-system) build tool and install it
set(SBOM_PROJECT_NAME "005-non-system-build-tool-installed")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_ADD_BUILD_TOOLS
)
sbom_test_record_project()

add_executable(build_tool_installed)
target_sources(build_tool_installed PRIVATE sources/tool_main.cpp)
install(TARGETS build_tool_installed
    BUNDLE DESTINATION bin
)
_qt_internal_add_sbom(build_tool_installed
    TYPE "BUILD_TOOL"
    RUNTIME_PATH bin
)

_qt_internal_sbom_end_project()

# Case 6, add a (non-system) build tool but don't install it
set(SBOM_PROJECT_NAME "006-non-system-build-tool-not-installed")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_ADD_BUILD_TOOLS
)
sbom_test_record_project()

add_executable(build_tool_not_installed)
target_sources(build_tool_not_installed PRIVATE sources/tool_main.cpp)
_qt_internal_add_sbom(build_tool_not_installed
    TYPE "BUILD_TOOL"
    NO_INSTALL
)

_qt_internal_sbom_end_project()

sbom_test_end()
