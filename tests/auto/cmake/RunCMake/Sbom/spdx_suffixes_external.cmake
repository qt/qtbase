# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

# Find the Main package and make its targets available. We will query
# the DocumentRef SPDX ids to check them.
find_package(Main REQUIRED)

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

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_VERSION "1.0.0")
set(SBOM_PROJECT_NAME "ExtSpdxSuffixes")

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)
sbom_test_record_project()

# Case 1, check that the exported DocumentRef id is the one we expect
# The ref is defined in case 003 of the spdx_suffixes test.
_qt_internal_sbom_get_external_document_ref_spdx_id_from_sbom_target(
    TARGET "Main::lib_003"
    OUT_VAR target_external_doc_ref_id
)
set(expected_ref "DocumentRef-003-custom-suffix-my-custom-suffix")
if(QT_GENERATE_SBOM AND NOT "${target_external_doc_ref_id}" STREQUAL "${expected_ref}")
    message(FATAL_ERROR
        "Expected external document ref id for target 'Main::lib_003' to be '${expected_ref}', "
        "but got '${target_external_doc_ref_id}'")
endif()

# Case 2, check that if we set the document ref id to empty to simulate as if it was never
# exported, adding a relationship on such a target still works due to temporary auto-creation of
# a ref id.
set_target_properties(Main::lib_003 PROPERTIES
    _qt_sbom_spdx_v2_external_document_ref ""
    _qt_sbom_spdx_v2_external_spdx_id ""
)
_qt_internal_sbom_get_spdx_id_for_target(Main::lib_003 lib_003_spdx_id)

create_sbom_lib_target(c1)
target_link_libraries(c1 PRIVATE Main::lib_003)

# First end the project, so that the temporary external doc ref id is generated.
_qt_internal_sbom_end_project()

# Now we can query the doc ref id to check it.
get_target_property(external_doc_ref_id Main::lib_003 _qt_sbom_spdx_v2_external_document_ref)
add_assert_str_exists_in_spdx_v2_3_doc(
    "Relationship: ${c1_spdx_id} DEPENDS_ON ${external_doc_ref_id}:${lib_003_spdx_id}")
add_cydx_v1_6_deps_to_result_file(c1 DEPS "${lib_003_spdx_id}")

sbom_test_end()

