# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_VERSION "1.0.0")

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)
sbom_test_record_project()

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

# Asserts that the given target is part of the document, but without the default
# 'project package CONTAINS the target package' relationship.
function(assert_project_does_not_have_contains_relationship_for_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_sbom_get_spdx_id_for_target(${target} target_spdx_id)
    add_assert_str_exists_in_spdx_v2_3_doc("SPDXID: ${target_spdx_id}")
    add_assert_str_not_exists_in_spdx_v2_3_doc(
        "Relationship: ${project_spdx_id} CONTAINS ${target_spdx_id}")
endfunction()

# Case 3
# A regular library gets the default 'project package CONTAINS the target package' relationship.
add_library(normal_lib STATIC)
target_sources(normal_lib PRIVATE sources/utils_helper.cpp)
install(TARGETS normal_lib ARCHIVE DESTINATION lib)
_qt_internal_add_sbom(normal_lib
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)
if(QT_GENERATE_SBOM)
    _qt_internal_sbom_get_spdx_id_for_target(normal_lib normal_lib_spdx_id)
    add_assert_str_exists_in_spdx_v2_3_doc(
        "Relationship: ${project_spdx_id} CONTAINS ${normal_lib_spdx_id}")
endif()

# Case 4
# A system library does not get the 'project contains' relationship, but is still part of the
# document.
_qt_internal_add_sbom(sys_lib
    SBOM_ENTITY_TYPE SYSTEM_LIBRARY
)
assert_project_does_not_have_contains_relationship_for_target(sys_lib)

# Case 5
# A system build tool does not get the 'project contains' relationship, but is still part of the
# document.
_qt_internal_add_sbom(sys_build_tool
    SBOM_ENTITY_TYPE SYSTEM_BUILD_TOOL
)
assert_project_does_not_have_contains_relationship_for_target(sys_build_tool)

# Case 6
# A regular library can opt out of the 'project contains' relationship.
add_library(optout_lib STATIC)
target_sources(optout_lib PRIVATE sources/utils_helper.cpp)
install(TARGETS optout_lib ARCHIVE DESTINATION lib)
_qt_internal_add_sbom(optout_lib
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
    NO_DEFAULT_PROJECT_CONTAINS_RELATIONSHIP
)
assert_project_does_not_have_contains_relationship_for_target(optout_lib)

_qt_internal_sbom_end_project()

sbom_test_end()
