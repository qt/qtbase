# Records a string that is expected to exist in the current SPDX v2.3 document.
function(add_assert_str_exists_in_spdx_v2_3_doc needle)
    # Only add when a document is actually generated.
    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_SPDX_V2)
        return()
    endif()

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_${doc_id}_spdx_needles "${needle}")
endfunction()

# Records a string that is expected NOT to exist in the current SPDX v2.3 document.
function(add_assert_str_not_exists_in_spdx_v2_3_doc needle)
    # Only add when a document is actually generated.
    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_SPDX_V2)
        return()
    endif()

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_${doc_id}_spdx_absent_needles "${needle}")
endfunction()

# Records a string that is expected to exist in the current CycloneDX v1.6 document.
function(add_assert_str_exists_in_cydx_v1_6_doc needle)
    # Only add when a document is actually generated.
    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_CYDX_V1_6)
        return()
    endif()

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_${doc_id}_cydx_needles "${needle}")
endfunction()

# Records a string that is expected NOT to exist in the current CycloneDX v1.6 document.
function(add_assert_str_not_exists_in_cydx_v1_6_doc needle)
    # Only add when a document is actually generated.
    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_CYDX_V1_6)
        return()
    endif()

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_${doc_id}_cydx_absent_needles "${needle}")
endfunction()

# Records a target, its spdx id and category in global properties for the current document.
# Used by the check.cmake infrastructure to do checks.
function(add_known_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        CATEGORY
        SPDX_ID
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_${doc_id}_known_targets "${target}")

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR
            "add_known_target: SPDX_ID argument is required for target '${target}'.")
    endif()

    set_property(GLOBAL PROPERTY
        _sbom_test_doc_${doc_id}_target_${target}_spdx_id "${arg_SPDX_ID}")

    if(arg_CATEGORY)
        set_property(GLOBAL APPEND PROPERTY
            _sbom_test_doc_${doc_id}_category_${arg_CATEGORY}_known_targets "${target}")

        get_property(categories GLOBAL PROPERTY _sbom_test_doc_${doc_id}_categories)
        if(NOT categories)
            set(categories "")
        endif()
        list(APPEND categories "${arg_CATEGORY}")
        set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_categories "${categories}")
    endif()
endfunction()

# Records the expected CycloneDX dependency spdx ids for a known target in the current document.
function(add_cydx_v1_6_deps_to_result_file target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args
        DEPS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    sbom_test_current_doc_id_or_error(doc_id)
    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_target_${target}_deps "${arg_DEPS}")
endfunction()

# Records various state for testing the current build's SBOM.
# Called once per whole CMake invocation.
function(sbom_test_begin)
    get_property(generated_project_spdx_ids GLOBAL PROPERTY
        _qt_internal_sbom_generated_project_spdx_ids)
    if(NOT generated_project_spdx_ids)
        set(generated_project_spdx_ids "")
    endif()
    list(LENGTH generated_project_spdx_ids previous_project_count)

    set_property(GLOBAL PROPERTY _sbom_test_previous_project_count "${previous_project_count}")
    set_property(GLOBAL PROPERTY _sbom_test_recorded_spdx_ids "")
    set_property(GLOBAL PROPERTY _sbom_test_doc_ids "")
    set_property(GLOBAL PROPERTY _sbom_test_current_doc_id "")
endfunction()

# Call once immediately after each _qt_internal_sbom_begin_project().
# Records the document paths of the active project under a document ID based on the lowercased SBOM
# project name.
function(sbom_test_record_project)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    get_property(doc_id GLOBAL PROPERTY _qt_internal_sbom_repo_project_name_lowercase)
    string(TOLOWER "${doc_id}" doc_id)

    _qt_internal_sbom_get_current_project_spdx_id(spdx_id)

    get_property(recorded_spdx_ids GLOBAL PROPERTY _sbom_test_recorded_spdx_ids)
    if(spdx_id AND spdx_id IN_LIST recorded_spdx_ids)
        message(FATAL_ERROR
            "sbom_test_record_project: a project with spdx id '${spdx_id}' was already recorded. "
            "Call sbom_test_record_project exactly once after each "
            "_qt_internal_sbom_begin_project.")
    endif()

    get_property(doc_ids GLOBAL PROPERTY _sbom_test_doc_ids)
    if(doc_id IN_LIST doc_ids)
        message(FATAL_ERROR
            "sbom_test_record_project: a project with id '${doc_id}' was already recorded. "
            "Document IDs are based on SBOM project names and need to be unique.")
    endif()

    set_property(GLOBAL APPEND PROPERTY _sbom_test_recorded_spdx_ids "${spdx_id}")
    set_property(GLOBAL APPEND PROPERTY _sbom_test_doc_ids "${doc_id}")
    set_property(GLOBAL PROPERTY _sbom_test_current_doc_id "${doc_id}")

    _qt_internal_sbom_get_current_project_document_path(spdx_rel FORMAT SPDX_V2_TAG_VALUE)
    _qt_internal_sbom_get_current_project_document_path(spdx_json_rel FORMAT SPDX_V2_JSON)
    _qt_internal_sbom_get_current_project_document_path(cydx_rel FORMAT CYDX_V1_6_JSON)

    set(spdx_abs "")
    set(spdx_json_abs "")
    set(cydx_abs "")
    set(present "")
    set(absent "")

    if(QT_GENERATE_SBOM)
        if(spdx_rel)
            set(spdx_abs "${CMAKE_INSTALL_PREFIX}/${spdx_rel}")
        endif()

        if(spdx_json_rel)
            set(spdx_json_abs "${CMAKE_INSTALL_PREFIX}/${spdx_json_rel}")
        endif()

        if(cydx_rel)
            set(cydx_abs "${CMAKE_INSTALL_PREFIX}/${cydx_rel}")
        endif()

        # FORMAT_CASE is a cache var.
        if(FORMAT_CASE STREQUAL "spdx23" OR FORMAT_CASE STREQUAL "all")
            if(QT_SBOM_GENERATE_SPDX_V2)
                list(APPEND present "${spdx_abs}")
            else()
                list(APPEND absent "${spdx_abs}")
            endif()

            if(QT_SBOM_GENERATE_SPDX_V2_JSON)
                list(APPEND present "${spdx_json_abs}")
            else()
                list(APPEND absent "${spdx_json_abs}")
            endif()
        endif()

        if(FORMAT_CASE STREQUAL "cydx16" OR FORMAT_CASE STREQUAL "all")
            if(QT_SBOM_GENERATE_CYDX_V1_6)
                list(APPEND present "${cydx_abs}")
            else()
                list(APPEND absent "${cydx_abs}")
            endif()
        endif()
    endif()

    # if QT_GENERATE_SBOM == OFF, present and absent stay empty, absence is verified by
    # sbom_check_none_case_absence.

    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_spdx "${spdx_abs}")
    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_spdx_json "${spdx_json_abs}")
    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_cydx "${cydx_abs}")
    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_present "${present}")
    set_property(GLOBAL PROPERTY _sbom_test_doc_${doc_id}_absent "${absent}")
endfunction()

# Returns the id of the current document; errors if none has been recorded yet.
function(sbom_test_current_doc_id_or_error out_var)
    get_property(doc_id GLOBAL PROPERTY _sbom_test_current_doc_id)

    if(NOT doc_id)
        message(FATAL_ERROR
            "No current SBOM document. Call sbom_test_record_project() after "
            "_qt_internal_sbom_begin_project().")
    endif()

    set(${out_var} "${doc_id}" PARENT_SCOPE)
endfunction()

# Checks that every project was recorded.
function(sbom_check_all_projects_recorded)
    get_property(generated_project_spdx_ids GLOBAL PROPERTY
        _qt_internal_sbom_generated_project_spdx_ids)
    get_property(previous_project_count GLOBAL PROPERTY _sbom_test_previous_project_count)
    get_property(recorded_spdx_ids GLOBAL PROPERTY _sbom_test_recorded_spdx_ids)
    list(LENGTH generated_project_spdx_ids generated_project_count)

    message(TRACE "Generated project spdx ids: ${generated_project_spdx_ids}")
    message(TRACE "Generated project count:    ${generated_project_count}")
    message(TRACE "Previous project count:     ${previous_project_count}")
    message(TRACE "Recorded project spdx ids:  ${recorded_spdx_ids}")

    if(previous_project_count STREQUAL "")
        message(FATAL_ERROR
            "sbom_check_all_projects_recorded: _sbom_test_previous_project_count "
            "property not set. Call sbom_test_begin() before sbom_test_record_project().")
    endif()

    set(missing "")
    set(index "${previous_project_count}")
    while(index LESS generated_project_count)
        list(GET generated_project_spdx_ids "${index}" begun_id)
        if(NOT begun_id IN_LIST recorded_spdx_ids)
            list(APPEND missing "${begun_id}")
        endif()
        math(EXPR index "${index} + 1")
    endwhile()

    if(missing)
        message(FATAL_ERROR
            "sbom_test_end: the following SBOM projects were begun but not recorded with "
            "sbom_test_record_project(): ${missing}. Call sbom_test_record_project() immediately "
            "after each _qt_internal_sbom_begin_project().")
    endif()
endfunction()

# Writes the recorded state for testing the current build's SBOM into a result.cmake file.
# Called once per whole CMake invocation.
function(sbom_test_end)
    sbom_check_all_projects_recorded()

    # Build the result.cmake content.
    get_property(doc_ids GLOBAL PROPERTY _sbom_test_doc_ids)

    set(content
"
set(EXPECTED_QT_GENERATE_SBOM \"${EXPECTED_QT_GENERATE_SBOM}\")
set(EXPECTED_QT_SBOM_GENERATE_SPDX_V2 \"${EXPECTED_QT_SBOM_GENERATE_SPDX_V2}\")
set(EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6 \"${EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6}\")
set(RESULT_QT_GENERATE_SBOM \"${QT_GENERATE_SBOM}\")
set(RESULT_QT_SBOM_GENERATE_SPDX_V2 \"${QT_SBOM_GENERATE_SPDX_V2}\")
set(RESULT_QT_SBOM_GENERATE_CYDX_V1_6 \"${QT_SBOM_GENERATE_CYDX_V1_6}\")
set(SBOM_DOCUMENT_IDS \"${doc_ids}\")
")

    set(prop_suffixes
        spdx
        spdx_json
        cydx
        present
        absent
        spdx_needles
        spdx_absent_needles
        cydx_needles
        cydx_absent_needles
        known_targets
    )

    foreach(doc_id IN LISTS doc_ids)
        foreach(suffix IN LISTS prop_suffixes)
            get_property(val GLOBAL PROPERTY _sbom_test_doc_${doc_id}_${suffix})
            string(TOUPPER "${suffix}" suffix_upper)
            string(APPEND content "set(SBOM_DOC_${doc_id}_${suffix_upper} \"${val}\")\n")
        endforeach()

        get_property(known_targets GLOBAL PROPERTY _sbom_test_doc_${doc_id}_known_targets)
        foreach(target IN LISTS known_targets)
            get_property(target_spdx_id GLOBAL PROPERTY
                _sbom_test_doc_${doc_id}_target_${target}_spdx_id)
            if(target_spdx_id)
                string(APPEND content
                    "set(SBOM_DOC_${doc_id}_TARGET_${target}_SPDX_ID \"${target_spdx_id}\")\n")
            endif()

            get_property(target_deps GLOBAL PROPERTY _sbom_test_doc_${doc_id}_target_${target}_deps)
            if(target_deps)
                string(APPEND content
                    "set(SBOM_DOC_${doc_id}_TARGET_${target}_DEPS \"${target_deps}\")\n")
            endif()
        endforeach()

        get_property(categories GLOBAL PROPERTY _sbom_test_doc_${doc_id}_categories)
        string(APPEND content "set(SBOM_DOC_${doc_id}_CATEGORIES \"${categories}\")\n")

        foreach(category IN LISTS categories)
            get_property(cat_targets GLOBAL PROPERTY
                _sbom_test_doc_${doc_id}_category_${category}_known_targets)
            string(APPEND content
                "set(SBOM_DOC_${doc_id}_CATEGORY_${category}_KNOWN_TARGETS \"${cat_targets}\")\n")
        endforeach()
    endforeach()

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/result.cmake" CONTENT "${content}")
endfunction()
