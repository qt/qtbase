# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# These variables are used by sbom_test_end().
set(SBOM_VERSION "1.0.0")
set(SBOM_PROJECT_NAME "TargetRels")

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)

include(CMakePackageConfigHelpers)
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

    # The installation bits are needed for the follow-up test which creates relationships on
    # targets from external documents.
    install(EXPORT MainTargets
        NAMESPACE Main::
        DESTINATION lib/cmake/Main
    )
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MainConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MainConfig.cmake"
        INSTALL_DESTINATION lib/cmake/Main
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MainConfig.cmake
            DESTINATION lib/cmake/Main )

    # This is used the by spdx and cydx deps checking code.
    _qt_internal_sbom_get_spdx_id_for_target(${target} ${target}_spdx_id)
    set(${target}_spdx_id "${${target}_spdx_id}" PARENT_SCOPE)

    add_known_target(${target}
        CATEGORY "RELATIONSHIP"
        SPDX_ID "${${target}_spdx_id}"
    )

    bubble_up_extra_result_code()
endfunction()

foreach(idx RANGE 1 10)
    set(target "t${idx}")
    create_sbom_lib_target(${target})
endforeach()

# Case 1
# Check that the deprecated SPDX v2 specific SBOM_RELATIONSHIPS option still works.
# CYDX dep does not get added.
set(relationship "${t2_spdx_id} DEPENDS_ON ${t1_spdx_id}")
_qt_internal_extend_sbom(t2 SBOM_RELATIONSHIPS "${relationship}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t2_spdx_id} DEPENDS_ON ${t1_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t2 DEPS "")

# Case 2
# Check that the new SBOM_RELATIONSHIP_ENTRIES works.
_qt_internal_extend_sbom(t3
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t3"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t2"
            SBOM_RELATIONSHIP_COMMENT
                "t3 depends on t2"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t3_spdx_id} DEPENDS_ON ${t2_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("RelationshipComment: <text>t3 depends on t2</text>")
add_cydx_v1_6_deps_to_result_file(t3 DEPS "${t2_spdx_id}")

# Case 3
# Check that multiple to relationships can be added.
_qt_internal_extend_sbom(t4
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t4"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t1"
                "t2"
                "t3"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t4_spdx_id} DEPENDS_ON ${t1_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t4_spdx_id} DEPENDS_ON ${t2_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t4_spdx_id} DEPENDS_ON ${t3_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t4 DEPS ${t1_spdx_id} ${t2_spdx_id} ${t3_spdx_id})

# Case 4
# Check that using both SBOM_RELATIONSHIP_ENTRIES and SBOM_RELATIONSHIPS works.
# The SBOM_RELATIONSHIPS value does not get added to CYDX.
set(relationship "${t5_spdx_id} DEPENDS_ON ${t1_spdx_id}")
_qt_internal_extend_sbom(t5
    SBOM_RELATIONSHIPS "${relationship}"
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t5"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t2"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t5_spdx_id} DEPENDS_ON ${t1_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t5_spdx_id} DEPENDS_ON ${t2_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t5 DEPS ${t2_spdx_id})

# Case 5
# Check that duplicate entries are removed.
_qt_internal_extend_sbom(t6
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t6"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t1"
                "t1"
                "t1"
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t6"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t2"
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t6"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t2"
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t6"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t3"
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t6_spdx_id} DEPENDS_ON ${t1_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t6_spdx_id} DEPENDS_ON ${t2_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t6_spdx_id} DEPENDS_ON ${t3_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t6 DEPS ${t1_spdx_id} ${t2_spdx_id} ${t3_spdx_id})

# Case 6
# Check that we can add relationships for specific formats only.
_qt_internal_extend_sbom(t7
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t7"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t1"
            SBOM_FORMATS SPDX_V2
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t7_spdx_id} DEPENDS_ON ${t1_spdx_id}")

_qt_internal_extend_sbom(t8
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t8"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t1"
            SBOM_FORMATS CYDX_V1_6
)
add_cydx_v1_6_deps_to_result_file(t8 DEPS ${t1_spdx_id})

_qt_internal_extend_sbom(t9
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                "t9"
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                "t1"
            SBOM_FORMATS SPDX_V2 CYDX_V1_6
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t9_spdx_id} DEPENDS_ON ${t1_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t9 DEPS ${t1_spdx_id})

# Case 7 check that manual setting of SBOM_DEPENDENCIES works
_qt_internal_extend_sbom(t10
    SBOM_DEPENDENCIES t8 t9
)
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t10_spdx_id} DEPENDS_ON ${t8_spdx_id}")
add_assert_str_exists_in_spdx_v2_3_doc("Relationship: ${t10_spdx_id} DEPENDS_ON ${t9_spdx_id}")
add_cydx_v1_6_deps_to_result_file(t10 DEPS ${t8_spdx_id} ${t9_spdx_id})

_qt_internal_sbom_end_project()

sbom_test_end()
