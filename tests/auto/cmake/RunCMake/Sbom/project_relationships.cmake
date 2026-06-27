# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# These variables are used by sbom_test_end().
set(SBOM_VERSION "1.0.0")

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)

function(create_sbom_lib_target target)
    add_library(${target} STATIC)
    target_sources(${target} PRIVATE sources/utils_helper.cpp)
    install(TARGETS ${target}
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

    bubble_up_extra_result_code()
endfunction()

foreach(idx RANGE 1 2)
    set(target "t${idx}")
    create_sbom_lib_target(${target})
    _qt_internal_sbom_get_spdx_id_for_target(${target} ${target}_spdx_id)
endforeach()

_qt_internal_sbom_get_current_project_spdx_id(project_spdx_id)
_qt_internal_sbom_get_current_project_target(project_target)

# Case 1
# Check that the new SBOM_RELATIONSHIP_ENTRIES works for project relationships.
_qt_internal_extend_sbom("${project_target}"
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t1"
            SBOM_RELATIONSHIP_TYPE
                DESCENDANT_OF
            SBOM_RELATIONSHIP_TO
                "${project_target}"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t1_spdx_id} DESCENDANT_OF ${project_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t1 DEPS "${project_spdx_id}")

# Case 2
# Check that multiple to relationships can be added.
_qt_internal_extend_sbom("${project_target}"
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t2"
            SBOM_RELATIONSHIP_TYPE
                BUILD_DEPENDENCY_OF # not really true, but good for testing
            SBOM_RELATIONSHIP_TO
                "t1"
                "${project_target}"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t2_spdx_id} BUILD_DEPENDENCY_OF ${t1_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t2_spdx_id} BUILD_DEPENDENCY_OF ${project_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t2 DEPS ${project_spdx_id} ${t1_spdx_id})

_qt_internal_sbom_end_project()

sbom_test_end()
