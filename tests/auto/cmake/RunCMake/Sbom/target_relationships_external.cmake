# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

# Find the Main package and make its targets available. We will query
# their SPDX ids to manually create external SBOM targets and relationships on them.
find_package(Main REQUIRED)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# These variables are used by sbom_test_end().
set(SBOM_VERSION "1.0.0")
set(SBOM_PROJECT_NAME "ExtTargetRels")

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
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

function(get_target_prop_or_error out_var target prop)
    get_target_property(result "${target}" "${prop}")
    if(NOT result)
        message(FATAL_ERROR "Target '${target}' does not have property '${prop}'")
    endif()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Query the external document id and other info from one of the Main package targets, so we can
# manually add external reference documents.
set(first_main_target "Main::t1")

if(QT_GENERATE_SBOM)
    get_target_prop_or_error(spdx_external_document_ref_id "${first_main_target}"
        _qt_sbom_spdx_v2_external_document_ref)

    get_target_prop_or_error(spdx_document_namespace "${first_main_target}"
        _qt_sbom_spdx_repo_document_namespace)

    get_target_prop_or_error(cydx_bom_serial_number "${first_main_target}"
        _qt_sbom_cydx_bom_serial_number_uuid)

    get_target_prop_or_error(cydx_urn_bom_version "${first_main_target}"
        _qt_sbom_cydx_external_urn_bom_version)

    get_target_prop_or_error(spdx_v2_tag_value_path "${first_main_target}"
        _qt_sbom_spdx_v2_document_tag_value_relative_path)

    get_target_prop_or_error(spdx_v2_json_path "${first_main_target}"
        _qt_sbom_spdx_v2_document_json_relative_path)

    get_target_prop_or_error(cydx_v1_6_json_path "${first_main_target}"
        _qt_sbom_cydx_v1_6_document_json_relative_path)
endif()

# Case 1
# Manually add spdx and cydx format external documents to the same target.
_qt_internal_sbom_add_external_reference_document(ExtDoc1
    SBOM_FORMAT "SPDX_V2_TAG_VALUE"
    SPDX_V2_DOCUMENT_REF_ID "${spdx_external_document_ref_id}"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${spdx_v2_tag_value_path}"
)

_qt_internal_sbom_add_external_reference_document(ExtDoc1
    SBOM_FORMAT "SPDX_V2_JSON"
    SPDX_V2_DOCUMENT_REF_ID "${spdx_external_document_ref_id}"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${spdx_v2_json_path}"
)

_qt_internal_sbom_add_external_reference_document(ExtDoc1
    SBOM_FORMAT "CYDX_V1_JSON"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${cydx_v1_6_json_path}"
)

# Case 2
# Manually add spdx and cydx format external documents to separate targets.
_qt_internal_sbom_add_external_reference_document(ExtDoc2Spdx
    SBOM_FORMAT "SPDX_V2_TAG_VALUE"
    SPDX_V2_DOCUMENT_REF_ID "${spdx_external_document_ref_id}-v2"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${spdx_v2_tag_value_path}"
)

_qt_internal_sbom_add_external_reference_document(ExtDoc3Cydx
    SBOM_FORMAT "CYDX_V1_JSON"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${cydx_v1_6_json_path}"
)

# Case 3
# Add the same docs, but instead of parsing the documents to extract the spdx namespace, sha1,
# and cydx serial numbers, specify them explicitly.
# For SPDX, we need to provide the SHA1, so we have to compute it first from the file path.
set(spdx_v2_sha1 "EMPTY_SHA")
if(QT_SBOM_GENERATE_SPDX_V2)
    _qt_internal_sbom_get_external_reference_search_paths(document_search_paths)
    _qt_internal_sbom_find_external_reference_document(spdx_v2_tag_value_absolute_path
        EXTERNAL_DOCUMENT_FILE_PATH "${spdx_v2_tag_value_path}"
        EXTERNAL_DOCUMENT_SEARCH_PATHS "${document_search_paths}"
    )
    if(QT_GENERATE_SBOM)
        file(SHA1 "${spdx_v2_tag_value_absolute_path}" spdx_v2_sha1)
    endif()
endif()

_qt_internal_sbom_add_external_reference_document(ExtDoc4
    SBOM_FORMAT "SPDX_V2_TAG_VALUE"
    SPDX_V2_DOCUMENT_REF_ID "${spdx_external_document_ref_id}-v4"
    SPDX_V2_DOCUMENT_NAMESPACE "${spdx_document_namespace}"
    SPDX_V2_DOCUMENT_SHA1 "${spdx_v2_sha1}"
)

_qt_internal_sbom_add_external_reference_document(ExtDoc4
    SBOM_FORMAT "CYDX_V1_JSON"
    CYDX_URN_SERIAL_NUMBER "${cydx_bom_serial_number}"
    CYDX_URN_BOM_VERSION "${cydx_urn_bom_version}"
)

# Case 4
# Add reference to a SPDX v2.3 JSON document.
# It should be able to parse out the namespace out of the document.
_qt_internal_sbom_add_external_reference_document(ExtDoc5
    SBOM_FORMAT "SPDX_V2_JSON"
    SPDX_V2_DOCUMENT_REF_ID "${spdx_external_document_ref_id}-v5"
    EXTERNAL_DOCUMENT_FILE_PATH
        "${spdx_v2_json_path}"
)

# Case 5
# Query the various external document properties and confirm they have expected values.
# The main_target extracted values are just relative paths, but the values set by the function
# below will be either -NOTFOUND or relative or absolute paths.
_qt_internal_sbom_query_external_reference_document(ExtDoc1
    OUT_VAR_PROJECT_NAME_LOWERCASE extdoc1_project_name_lowercase

    OUT_VAR_RELATIVE_SPDX_V2_TAG_VALUE_DOCUMENT_PATH extdoc1_spdx_v2_tag_value_relative_path
    OUT_VAR_RELATIVE_SPDX_V2_JSON_DOCUMENT_PATH extdoc1_spdx_v2_json_relative_path
    OUT_VAR_RELATIVE_CYDX_V1_6_JSON_DOCUMENT_PATH extdoc1_cydx_v1_6_json_relative_path

    OUT_VAR_SPDX_V2_DOCUMENT_REF_ID extdoc1_spdx_v2_document_ref_id
    OUT_VAR_SPDX_V2_DOCUMENT_NAMESPACE extdoc1_spdx_v2_document_namespace

    OUT_VAR_CYDX_URN_SERIAL_NUMBER extdoc1_cydx_urn_serial_number
    OUT_VAR_CYDX_URN_BOM_VERSION extdoc1_cydx_urn_bom_version
)

if(QT_GENERATE_SBOM AND QT_SBOM_GENERATE_SPDX_V2)
    if(NOT extdoc1_spdx_v2_tag_value_relative_path
            OR NOT extdoc1_spdx_v2_tag_value_relative_path MATCHES "${spdx_v2_tag_value_path}")
        message(FATAL_ERROR "Expected relative path '${spdx_v2_tag_value_path}' but got "
            "'${extdoc1_spdx_v2_tag_value_relative_path}'")
    endif()

    if(NOT extdoc1_spdx_v2_json_relative_path
            OR NOT extdoc1_spdx_v2_json_relative_path MATCHES "${spdx_v2_json_path}")
        message(FATAL_ERROR "Expected relative path '${spdx_v2_json_path}' but got "
            "'${extdoc1_spdx_v2_json_relative_path}'")
    endif()

    if(NOT extdoc1_spdx_v2_document_ref_id
            OR NOT extdoc1_spdx_v2_document_ref_id MATCHES "${spdx_external_document_ref_id}")
        message(FATAL_ERROR
            "Expected SPDX v2 document ref id '${spdx_external_document_ref_id}' but got "
            "'${extdoc1_spdx_v2_document_ref_id}'")
    endif()

    if(NOT extdoc1_spdx_v2_document_namespace
            OR NOT extdoc1_spdx_v2_document_namespace MATCHES "${spdx_document_namespace}")
        message(FATAL_ERROR
            "Expected SPDX v2 document namespace '${spdx_document_namespace}' but got"
            " '${extdoc1_spdx_v2_document_namespace}'")
    endif()
endif()

if(QT_GENERATE_SBOM AND QT_SBOM_GENERATE_CYDX_V1_6)
    if(NOT extdoc1_cydx_v1_6_json_relative_path
            OR NOT extdoc1_cydx_v1_6_json_relative_path MATCHES "${cydx_v1_6_json_path}")
        message(FATAL_ERROR "Expected relative path '${cydx_v1_6_json_path}' but got "
            "'${extdoc1_cydx_v1_6_json_relative_path}'")
    endif()

    if(NOT extdoc1_cydx_urn_serial_number
            OR NOT extdoc1_cydx_urn_serial_number MATCHES "${cydx_bom_serial_number}")
        message(FATAL_ERROR "Expected CYDX urn serial number '${cydx_bom_serial_number}' but got "
            "'${extdoc1_cydx_urn_serial_number}'")
    endif()

    if(NOT extdoc1_cydx_urn_bom_version
            OR NOT extdoc1_cydx_urn_bom_version MATCHES "${cydx_urn_bom_version}")
        message(FATAL_ERROR "Expected CYDX urn bom version '${cydx_urn_bom_version}' but got "
            "'${extdoc1_cydx_urn_bom_version}'")
    endif()
endif()

# External sbom targets test cases.

foreach(idx RANGE 1 3)
    # c for consumer
    set(target "c${idx}")
    create_sbom_lib_target(${target})
endforeach()

foreach(idx RANGE 1 3)
    set(main_target "Main::t${idx}")
    # mt for main target
    set(main_prefix "mt${idx}")
    _qt_internal_sbom_get_spdx_id_for_target(${main_target} ${main_prefix}_spdx_id)
endforeach()

# Case 1
# Create an target representing an external SBOM package / component.
# An external SBOM target is always associated with an external reference document.
# We extract and use the spdx id from Main's exported targets, basically recreating them manually
# as external.
_qt_internal_add_sbom(t1_ext
    SPDX_ID "${mt1_spdx_id}"
    IS_EXTERNAL_SBOM_ENTITY
    EXTERNAL_SBOM_DOCUMENT_TARGET ExtDoc1
    SBOM_ENTITY_TYPE LIBRARY
)

# Case 2
# Add a relationship on the external target. This adds a relationship for both spdx and cydx
# formats, because the external document target has info for both.
_qt_internal_extend_sbom(c1
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                c1
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                t1_ext
            SBOM_RELATIONSHIP_COMMENT
                "c1 depends on t1_ext"
)
add_assert_str_exists_in_spdx_v2_3_doc(
    "Relationship: ${c1_spdx_id} DEPENDS_ON ${extdoc1_spdx_v2_document_ref_id}:${mt1_spdx_id}")
add_cydx_v1_6_deps_to_result_file(c1 DEPS "${mt1_spdx_id}")

# Case 3
# Add relationship to an external target that is only defined in one format.
_qt_internal_add_sbom(t2_ext
    SPDX_ID "${mt2_spdx_id}"
    IS_EXTERNAL_SBOM_ENTITY
    EXTERNAL_SBOM_DOCUMENT_TARGET ExtDoc2Spdx
    SBOM_ENTITY_TYPE LIBRARY
)
_qt_internal_add_sbom(t3_ext
    SPDX_ID "${mt3_spdx_id}"
    IS_EXTERNAL_SBOM_ENTITY
    EXTERNAL_SBOM_DOCUMENT_TARGET ExtDoc3Cydx
    SBOM_ENTITY_TYPE LIBRARY
)

_qt_internal_extend_sbom(c2
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                c2
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                t2_ext
            SBOM_RELATIONSHIP_COMMENT
                "c2 depends on t2_ext"
            SBOM_FORMATS SPDX_V2
)
_qt_internal_sbom_query_external_reference_document(ExtDoc2Spdx
    OUT_VAR_SPDX_V2_DOCUMENT_REF_ID extdoc2_spdx_v2_document_ref_id
)
add_assert_str_exists_in_spdx_v2_3_doc(
    "Relationship: ${c2_spdx_id} DEPENDS_ON ${extdoc2_spdx_v2_document_ref_id}:${mt2_spdx_id}")

_qt_internal_extend_sbom(c3
    SBOM_RELATIONSHIP_ENTRIES
        SBOM_RELATIONSHIP_ENTRY
            SBOM_RELATIONSHIP_FROM
                c3
            SBOM_RELATIONSHIP_TYPE
                DEPENDS_ON
            SBOM_RELATIONSHIP_TO
                t3_ext
            SBOM_RELATIONSHIP_COMMENT
                "c3 depends on t3_ext"
            SBOM_FORMATS CYDX_V1_6
)
add_cydx_v1_6_deps_to_result_file(c3 DEPS "${mt3_spdx_id}")

_qt_internal_sbom_end_project()

sbom_test_end()
