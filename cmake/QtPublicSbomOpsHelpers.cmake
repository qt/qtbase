# Copyright (C) 2024 The Qt Company Ltd.
# Copyright (C) 2023-2024 Jochem Rutgers
# SPDX-License-Identifier: MIT AND BSD-3-Clause

# Handles the look up of Python, Python spdx dependencies and other various post-installation steps
# like NTIA validation, auditing, json generation, etc.
function(_qt_internal_sbom_setup_project_ops_generation)
    set(opt_args
        GENERATE_JSON
        GENERATE_JSON_REQUIRED
        GENERATE_SOURCE_SBOM
        VERIFY_SBOM
        VERIFY_SBOM_REQUIRED
        VERIFY_NTIA_COMPLIANT
        LINT_SOURCE_SBOM
        LINT_SOURCE_SBOM_NO_ERROR
        SHOW_TABLE
        AUDIT
        AUDIT_NO_ERROR
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_GENERATE_JSON AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(op_args
            OP_KEY "GENERATE_JSON"
            OUT_VAR_DEPS_FOUND deps_found
        )
        if(arg_GENERATE_JSON_REQUIRED)
            list(APPEND op_args REQUIRED)
        endif()

        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(${op_args})
        if(deps_found)
            _qt_internal_sbom_generate_json()
        endif()
    endif()

    if(arg_VERIFY_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(op_args
            OP_KEY "VERIFY_SBOM"
            OUT_VAR_DEPS_FOUND deps_found
        )
        if(arg_VERIFY_SBOM_REQUIRED)
            list(APPEND op_args REQUIRED)
        endif()

        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(${op_args})
        if(deps_found)
            _qt_internal_sbom_verify_valid()
        endif()
    endif()

    if(arg_VERIFY_NTIA_COMPLIANT AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(REQUIRED OP_KEY "RUN_NTIA")
        _qt_internal_sbom_verify_ntia_compliant()
    endif()

    if(arg_SHOW_TABLE AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_python_dependency_program(NAME sbom2doc REQUIRED)
        _qt_internal_sbom_show_table()
    endif()

    if(arg_AUDIT AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(audit_no_error_option "")
        if(arg_AUDIT_NO_ERROR)
            set(audit_no_error_option NO_ERROR)
        endif()
        _qt_internal_sbom_find_python_dependency_program(NAME sbomaudit REQUIRED)
        _qt_internal_sbom_audit(${audit_no_error_option})
    endif()

    if(arg_GENERATE_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_python_dependency_program(NAME reuse REQUIRED)
        _qt_internal_sbom_generate_reuse_source_sbom()
    endif()

    if(arg_LINT_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(lint_no_error_option "")
        if(arg_LINT_SOURCE_SBOM_NO_ERROR)
            set(lint_no_error_option NO_ERROR)
        endif()
        _qt_internal_sbom_find_python_dependency_program(NAME reuse REQUIRED)
        _qt_internal_sbom_run_reuse_lint(
            ${lint_no_error_option}
            BUILD_TIME_SCRIPT_PATH_OUT_VAR reuse_lint_script
        )
    endif()
endfunction()

# Helper to find a python interpreter and a specific python dependency, e.g. to be able to generate
# a SPDX JSON SBOM, or run post-installation steps like NTIA verification.
# The exact dependency should be specified as the OP_KEY.
#
# Caches the found python executable in a separate cache var QT_INTERNAL_SBOM_PYTHON_EXECUTABLE, to
# avoid conflicts with any other found python interpreter.
function(_qt_internal_sbom_find_and_handle_sbom_op_dependencies)
    set(opt_args
        REQUIRED
    )
    set(single_args
        OP_KEY
        OUT_VAR_DEPS_FOUND
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OP_KEY)
        message(FATAL_ERROR "OP_KEY is required")
    endif()

    set(supported_ops "GENERATE_JSON" "VERIFY_SBOM" "RUN_NTIA")

    if(arg_OP_KEY STREQUAL "GENERATE_JSON" OR arg_OP_KEY STREQUAL "VERIFY_SBOM")
        set(import_statement "import spdx_tools.spdx.clitools.pyspdxtools")
    elseif(arg_OP_KEY STREQUAL "RUN_NTIA")
        set(import_statement "import ntia_conformance_checker.main")
    else()
        message(FATAL_ERROR "OP_KEY must be one of ${supported_ops}")
    endif()

    # Return early if we found the dependencies.
    if(QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY})
        if(arg_OUT_VAR_DEPS_FOUND)
            set(${arg_OUT_VAR_DEPS_FOUND} TRUE PARENT_SCOPE)
        endif()
        return()
    endif()

    # NTIA-compliance checker requires Python 3.9 or later, so we use it as the minimum for all
    # SBOM OPs.
    set(required_version "3.9")

    set(python_common_args
        VERSION "${required_version}"
    )

    set(everything_found FALSE)

    # On macOS FindPython prefers looking in the system framework location, but that usually would
    # not have the required dependencies. So we first look in it, and then fallback to any other
    # non-framework python found.
    if(CMAKE_HOST_APPLE)
        set(extra_python_args SEARCH_IN_FRAMEWORKS QUIET)
        _qt_internal_sbom_find_python_and_dependency_helper_lambda()
    endif()

    if(NOT everything_found)
        set(extra_python_args QUIET)
        _qt_internal_sbom_find_python_and_dependency_helper_lambda()
    endif()

    # Always save the python interpreter path if it is found, even if the dependencies are not
    # found. This improves the error message workflow.
    if(python_found AND NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        set(QT_INTERNAL_SBOM_PYTHON_EXECUTABLE "${python_path}" CACHE INTERNAL
            "Python interpeter used for SBOM generation.")
    endif()

    if(NOT everything_found)
        if(arg_REQUIRED)
            set(message_type "FATAL_ERROR")
        else()
            set(message_type "DEBUG")
        endif()

        if(NOT python_found)
            # Look for python one more time, this time without QUIET, to show an error why it
            # wasn't found.
            if(arg_REQUIRED)
                _qt_internal_sbom_find_python_helper(${python_common_args}
                    OUT_VAR_PYTHON_PATH unused_python
                    OUT_VAR_PYTHON_FOUND unused_found
                )
            endif()
            message(${message_type} "Python ${required_version} for running SBOM ops not found.")
        elseif(NOT dep_found)
            message(${message_type} "Python dependency for running SBOM op ${arg_OP_KEY} "
                "not found:\n Python: ${python_path} \n Output: \n${dep_find_output}")
        endif()
    else()
        message(DEBUG "Using Python ${python_path} for running SBOM ops.")

        set(QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY} "TRUE" CACHE INTERNAL
            "All dependencies found to run SBOM OP ${arg_OP_KEY}")
    endif()

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY}}"
            PARENT_SCOPE)
    endif()
endfunction()

# Helper that checks a variable for truthiness, and sets the OUT_VAR_DEPS_FOUND and
# OUT_VAR_REASON_FAILURE_MESSAGE variables based on what was passed in.
function(_qt_internal_sbom_handle_sbom_op_missing_dependency)
    set(opt_args "")
    set(single_args
        VAR_TO_CHECK
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
    )
    set(multi_args
        FAILURE_MESSAGE
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_VAR_TO_CHECK)
        message(FATAL_ERROR "VAR_TO_CHECK is required")
    endif()

    set(reason "")
    set(value "${${arg_VAR_TO_CHECK}}")
    if(NOT value)
        if(arg_FAILURE_MESSAGE)
            list(JOIN arg_FAILURE_MESSAGE " " reason)
        endif()
        set(deps_found FALSE)
    else()
        set(deps_found TRUE)
    endif()

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_REASON_FAILURE_MESSAGE)
        set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE} "${reason}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to query whether the sbom python interpreter is available, and also the error message if it
# is not available.
function(_qt_internal_sbom_check_python_interpreter_available)
    set(opt_args "")
    set(single_args
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
    )
    set(multi_args
        FAILURE_MESSAGE_PREFIX
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(failure_message
        "QT_INTERNAL_SBOM_PYTHON_EXECUTABLE is missing a valid path to a python interpreter")

    if(arg_FAILURE_MESSAGE_PREFIX)
        list(PREPEND failure_message ${arg_FAILURE_MESSAGE_PREFIX})
    endif()

    _qt_internal_sbom_handle_sbom_op_missing_dependency(
        VAR_TO_CHECK QT_INTERNAL_SBOM_PYTHON_EXECUTABLE
        FAILURE_MESSAGE ${failure_message}
        OUT_VAR_DEPS_FOUND deps_found
        OUT_VAR_REASON_FAILURE_MESSAGE reason
    )

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_REASON_FAILURE_MESSAGE)
        set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE} "${reason}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to assert that the python interpreter is available.
function(_qt_internal_sbom_assert_python_interpreter_available error_message_prefix)
    _qt_internal_sbom_check_python_interpreter_available(
        FAILURE_MESSAGE_PREFIX ${error_message_prefix}
        OUT_VAR_DEPS_FOUND deps_found
        OUT_VAR_REASON_FAILURE_MESSAGE reason
    )

    if(NOT deps_found)
        message(FATAL_ERROR ${reason})
    endif()
endfunction()

# Helper to query whether an sbom python dependency is available, and also the error message if it
# is not available.
function(_qt_internal_sbom_check_python_dependency_available)
    set(opt_args "")
    set(single_args
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
        VAR_TO_CHECK
    )
    set(multi_args
        FAILURE_MESSAGE_PREFIX
        FAILURE_MESSAGE_SUFFIX
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(failure_message
        "Required Python dependencies not found: ")

    if(arg_FAILURE_MESSAGE_PREFIX)
        list(PREPEND failure_message ${arg_FAILURE_MESSAGE_PREFIX})
    endif()

    if(arg_FAILURE_MESSAGE_SUFFIX)
        list(APPEND failure_message ${arg_FAILURE_MESSAGE_SUFFIX})
    endif()

    _qt_internal_sbom_handle_sbom_op_missing_dependency(
        VAR_TO_CHECK ${arg_VAR_TO_CHECK}
        FAILURE_MESSAGE ${failure_message}
        OUT_VAR_DEPS_FOUND deps_found
        OUT_VAR_REASON_FAILURE_MESSAGE reason
    )

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_REASON_FAILURE_MESSAGE)
        set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE} "${reason}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to assert that an sbom python dependency is available.
function(_qt_internal_sbom_assert_python_dependency_available key dep error_message_prefix)
    _qt_internal_sbom_check_python_dependency_available(
        VAR_TO_CHECK QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${key}
        FAILURE_MESSAGE_PREFIX ${error_message_prefix}
        FAILURE_MESSAGE_SUFFIX ${dep}
        OUT_VAR_DEPS_FOUND deps_found
        OUT_VAR_REASON_FAILURE_MESSAGE reason
    )

    if(NOT deps_found)
        message(FATAL_ERROR ${reason})
    endif()
endfunction()

# Helper to query whether the json sbom generation dependency is available, and also the error
# message if it is not available.
function(_qt_internal_sbom_check_generate_json_dependency_available)
    set(opt_args "")
    set(single_args
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_check_python_dependency_available(
        VAR_TO_CHECK QT_INTERNAL_SBOM_DEPS_FOUND_FOR_GENERATE_JSON
        FAILURE_MESSAGE_SUFFIX "spdx_tools.spdx.clitools.pyspdxtools"
        OUT_VAR_DEPS_FOUND deps_found
        OUT_VAR_REASON_FAILURE_MESSAGE reason
    )

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
    endif()
    if(arg_OUT_VAR_REASON_FAILURE_MESSAGE)
        set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE} "${reason}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to generate a SPDX JSON file from a tag/value format file.
# This also implies some additional validity checks, useful to ensure a proper sbom file.
function(_qt_internal_sbom_generate_json)
    set(error_message_prefix "Failed to generate an SBOM json file.")
    _qt_internal_sbom_assert_python_interpreter_available("${error_message_prefix}")
    _qt_internal_sbom_assert_python_dependency_available(GENERATE_JSON
        "spdx_tools.spdx.clitools.pyspdxtools" ${error_message_prefix})

    set(content "
        message(STATUS \"Generating JSON: \${QT_SBOM_OUTPUT_PATH}.json\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m spdx_tools.spdx.clitools.pyspdxtools
            -i \"\${QT_SBOM_OUTPUT_PATH}\" -o \"\${QT_SBOM_OUTPUT_PATH}.json\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM conversion to JSON failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/convert_to_json.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to query whether the all required dependencies are available to generate a tag / value
# document from a json one.
function(_qt_internal_sbom_verify_deps_for_generate_tag_value_spdx_document)
    set(opt_args "")
    set(single_args
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # First try to find dependencies, because they might not have been found yet.
    _qt_internal_sbom_find_and_handle_sbom_op_dependencies(
        OP_KEY "GENERATE_JSON"
    )

    _qt_internal_sbom_check_python_interpreter_available(
        OUT_VAR_DEPS_FOUND python_found
        OUT_VAR_REASON_FAILURE_MESSAGE python_error
    )
    _qt_internal_sbom_check_generate_json_dependency_available(
        OUT_VAR_DEPS_FOUND dep_found
        OUT_VAR_REASON_FAILURE_MESSAGE dep_error
    )

    set(reasons "")
    if(python_error)
        string(APPEND reasons "${python_error}\n")
    endif()
    if(dep_error)
        string(APPEND reasons "${dep_error}\n")
    endif()

    if(python_found AND dep_found)
        set(deps_found "TRUE")
    else()
        set(deps_found "FALSE")
    endif()

    set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
    set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE} "${reasons}" PARENT_SCOPE)
endfunction()

# Helper to generate a tag/value SPDX file from a SPDX JSON format file.
#
# Will be used by WebEngine to convert the Chromium JSON file to a tag/value SPDX file.
#
# This conversion needs to happen before the document is referenced in the SBOM generation process,
# so that the file already exists when it is parsed for its unique id and namespace.
# It also needs to happen before verification codes are computed for the current document
# that will depend on the target one, to ensure the the file exists and its checksum can be
# computed.
#
# OPERATION_ID - a unique id for the operation, used to generate a unique cmake file name for
# the SBOM generation process.
#
# INPUT_JSON_PATH - the absolute path to the input JSON file.
#
# OUTPUT_FILE_PATH - the absolute path where to create the output tag/value SPDX file.
# Note that if the output file path is set, it is up to the caller to also copy / install the file
# into the build and install directories where the build system expects to find all external
# document references.
#
# OUTPUT_FILE_NAME - when OUTPUT_FILE_PATH is not specified, the output directory is automatically
# set to the SBOM output directory. In this case OUTPUT_FILE_NAME can be used to override the
# outout file name. If not specified, it will be derived from the input file name.
#
# OUT_VAR_OUTPUT_FILE_NAME - output variable where to store the output file.
#
# OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH - output variable where to store the output file path.
# Note that the path will contain an unresolved '${QT_SBOM_OUTPUT_DIR}' which only has a value at
# install time. So the path can't be used sensibly during configure time.
#
# OPTIONAL - whether the operation should return early, if the required python dependencies are
# not found. OUT_VAR_DEPS_FOUND is still set in that case.
#
# OUT_VAR_DEPS_FOUND - output variable where to store whether the python dependencies for the
# operation were found, and thus the conversion will be attempted.
function(_qt_internal_sbom_generate_tag_value_spdx_document)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        OPTIONAL
    )
    set(single_args
        OPERATION_ID
        INPUT_JSON_FILE_PATH
        OUTPUT_FILE_PATH
        OUTPUT_FILE_NAME
        OUT_VAR_OUTPUT_FILE_NAME
        OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH
        OUT_VAR_DEPS_FOUND
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # First try to find dependencies, because they might not have been found yet.
    _qt_internal_sbom_find_and_handle_sbom_op_dependencies(
        OP_KEY "GENERATE_JSON"
        OUT_VAR_DEPS_FOUND deps_found
    )

    if(arg_OPTIONAL)
        set(deps_are_required FALSE)
    else()
        set(deps_are_required TRUE)
    endif()

    # If the operation has to succeed, then the deps are required. Assert they are available.
    if(deps_are_required)
        set(error_message_prefix "Failed to generate a tag/value SBOM file from a json SBOM file.")
        _qt_internal_sbom_assert_python_interpreter_available("${error_message_prefix}")
        _qt_internal_sbom_assert_python_dependency_available(GENERATE_JSON
            "spdx_tools.spdx.clitools.pyspdxtools" ${error_message_prefix})

    # If the operation is optional, don't error out if the deps are not found, but silently return
    # and mention that the deps are not found.
    elseif(NOT deps_found)
        set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)
        return()
    endif()

    set(${arg_OUT_VAR_DEPS_FOUND} "${deps_found}" PARENT_SCOPE)

    if(NOT arg_OPERATION_ID)
        message(FATAL_ERROR "OPERATION_ID is required")
    endif()

    if(NOT arg_INPUT_JSON_FILE_PATH)
        message(FATAL_ERROR "INPUT_JSON_FILE_PATH is required")
    endif()

    if(arg_OUTPUT_FILE_PATH)
        set(output_path "${arg_OUTPUT_FILE_PATH}")
    else()
        if(arg_OUTPUT_FILE_NAME)
            set(output_name "${arg_OUTPUT_FILE_NAME}")
        else()
            # Use the input file name without the last extension (without .json) as the output name.
            get_filename_component(output_name "${arg_INPUT_JSON_FILE_PATH}" NAME_WLE)
        endif()
        set(output_path "\${QT_SBOM_OUTPUT_DIR}/${output_name}")
    endif()

    if(arg_OUT_VAR_OUTPUT_FILE_NAME)
        get_filename_component(output_name_resolved "${output_path}" NAME)
        set(${arg_OUT_VAR_OUTPUT_FILE_NAME} "${output_name_resolved}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH)
        set(${arg_OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH} "${output_path}" PARENT_SCOPE)
    endif()

    set(content "
        message(STATUS
            \"Generating tag/value SPDX document: ${output_path} from \"
        \"${arg_INPUT_JSON_FILE_PATH}\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m spdx_tools.spdx.clitools.pyspdxtools
            -i \"${arg_INPUT_JSON_FILE_PATH}\" -o \"${output_path}\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM conversion to tag/value failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(convert_sbom "${sbom_dir}/convert_to_tag_value_${arg_OPERATION_ID}.cmake")
    file(GENERATE OUTPUT "${convert_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files
        "${convert_sbom}")
endfunction()

# Helper to verify the generated sbom is valid.
function(_qt_internal_sbom_verify_valid)
    set(error_message_prefix "Failed to verify SBOM file syntax.")
    _qt_internal_sbom_assert_python_interpreter_available("${error_message_prefix}")
    _qt_internal_sbom_assert_python_dependency_available(VERIFY_SBOM
        "spdx_tools.spdx.clitools.pyspdxtools" ${error_message_prefix})

    set(content "
        message(STATUS \"Verifying: \${QT_SBOM_OUTPUT_PATH}\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m spdx_tools.spdx.clitools.pyspdxtools
            -i \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM verification failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/verify_valid.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to verify the generated sbom is NTIA compliant.
function(_qt_internal_sbom_verify_ntia_compliant)
    set(error_message_prefix "Failed to run NTIA checker on SBOM file.")
    _qt_internal_sbom_assert_python_interpreter_available("${error_message_prefix}")
    _qt_internal_sbom_assert_python_dependency_available(RUN_NTIA
        "ntia_conformance_checker.main" ${error_message_prefix})

    set(content "
        message(STATUS \"Checking for NTIA compliance: \${QT_SBOM_OUTPUT_PATH}\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m ntia_conformance_checker.main
            --file \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM NTIA verification failed: \{res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/verify_ntia.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to show the main sbom document info in the form of a CLI table.
function(_qt_internal_sbom_show_table)
    set(extra_code_begin "")
    if(DEFINED ENV{COIN_UNIQUE_JOB_ID})
        # The output of the process dynamically adjusts the width of the shown table based on the
        # console width. In the CI, the width is very short for some reason, and thus the output
        # is truncated in the CI log. Explicitly set a bigger width to avoid this.
        set(extra_code_begin "
set(backup_env_columns \$ENV{COLUMNS})
set(ENV{COLUMNS} 150)
")
set(extra_code_end "
set(ENV{COLUMNS} \${backup_env_columns})
")
    endif()

    set(content "
        message(STATUS \"Showing main SBOM document info: \${QT_SBOM_OUTPUT_PATH}\")

        ${extra_code_begin}
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_SBOM2DOC} -i \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        ${extra_code_end}
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"Showing SBOM document failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/show_table.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to audit the generated sbom.
function(_qt_internal_sbom_audit)
    set(opt_args NO_ERROR)
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"SBOM Audit failed: \${res}\")
            endif()
")
    endif()

    set(content "
        message(STATUS \"Auditing SBOM: \${QT_SBOM_OUTPUT_PATH}\")
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_SBOMAUDIT} -i \"\${QT_SBOM_OUTPUT_PATH}\"
                    --disable-license-check --cpecheck --offline
            RESULT_VARIABLE res
        )
        ${handle_error}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/audit.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Returns path to project's potential root source reuse.toml file.
function(_qt_internal_sbom_get_project_reuse_toml_path out_var)
    set(reuse_toml_path "${PROJECT_SOURCE_DIR}/REUSE.toml")
    set(${out_var} "${reuse_toml_path}" PARENT_SCOPE)
endfunction()

# Helper to generate and install a source SBOM using reuse.
function(_qt_internal_sbom_generate_reuse_source_sbom)
    set(opt_args NO_ERROR)
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_op "${sbom_dir}/generate_reuse_source_sbom.cmake")

    _qt_internal_sbom_get_project_reuse_toml_path(reuse_toml_path)
    if(NOT EXISTS "${reuse_toml_path}" AND NOT QT_FORCE_SOURCE_SBOM_GENERATION)
        set(skip_message
            "Skipping source SBOM generation: No reuse.toml file found at '${reuse_toml_path}'.")
        message(STATUS "${skip_message}")

        set(content "
            message(STATUS \"${skip_message}\")
")

        file(GENERATE OUTPUT "${file_op}" CONTENT "${content}")
        set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_post_generation_include_files
            "${file_op}")
        return()
    endif()

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"Source SBOM generation using reuse tool failed: \${res}\")
            endif()
")
    endif()

    set(source_sbom_path "\${QT_SBOM_OUTPUT_PATH_WITHOUT_EXT}.source.spdx")
    file(TO_CMAKE_PATH "$ENV{QT_QA_LICENSE_TEST_DIR}/$ENV{QT_SOURCE_SBOM_TEST_SCRIPT}"
        full_path_to_license_test)
    set(verify_source_sbom "
            if(res EQUAL 0)
                message(STATUS \"Verifying source SBOM ${source_sbom_path} using qtqa tst_licenses.pl ${full_path_to_license_test}\")
                if(NOT EXISTS \"${full_path_to_license_test}\")
                    message(FATAL_ERROR \"Source SBOM check has failed: The tst_licenses.pl script could not be found at ${full_path_to_license_test}\")
                endif()
                execute_process(
                    COMMAND perl \"\$ENV{QT_SOURCE_SBOM_TEST_SCRIPT}\" -sbomonly -sbom \"${source_sbom_path}\"
                    WORKING_DIRECTORY \"\$ENV{QT_QA_LICENSE_TEST_DIR}\"
                    RESULT_VARIABLE res
                    COMMAND_ECHO STDOUT
                )
                if(NOT res EQUAL 0)
                    message(FATAL_ERROR \"Source SBOM check has failed: \${res}\")
                endif()
            endif()
")

    set(extra_reuse_args "")

    get_property(project_supplier GLOBAL PROPERTY _qt_sbom_project_supplier)
    if(project_supplier)
        get_property(project_supplier_url GLOBAL PROPERTY _qt_sbom_project_supplier_url)

        # Add the supplier url if available. Add it in parenthesis to stop reuse from adding its
        # own empty parenthesis.
        if(project_supplier_url)
            set(project_supplier "${project_supplier} (${project_supplier_url})")
        endif()

        # Unfortunately there's no way to silence the addition of the 'Creator: Person' field,
        # even though 'Creator: Organization' is supplied.
        list(APPEND extra_reuse_args --creator-organization "${project_supplier}")
    endif()

    set(content "
        message(STATUS \"Generating source SBOM using reuse tool: ${source_sbom_path}\")
        set(extra_reuse_args \"${extra_reuse_args}\")
        execute_process(
            COMMAND
                ${QT_SBOM_PROGRAM_REUSE}
                --root \"${PROJECT_SOURCE_DIR}\"
                spdx
                -o ${source_sbom_path}
                \${extra_reuse_args}
            RESULT_VARIABLE res
        )
        ${handle_error}
        if(\"\$ENV{VERIFY_SOURCE_SBOM}\")
            ${verify_source_sbom}
        endif()
")

    file(GENERATE OUTPUT "${file_op}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_post_generation_include_files "${file_op}")
endfunction()

# Helper to run 'reuse lint' on the project source dir.
function(_qt_internal_sbom_run_reuse_lint)
    set(opt_args
        NO_ERROR
    )
    set(single_args
        BUILD_TIME_SCRIPT_PATH_OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # If no reuse.toml file exists, it means the repo is likely not reuse compliant yet,
    # so we shouldn't error out during installation when running the lint.
    _qt_internal_sbom_get_project_reuse_toml_path(reuse_toml_path)
    if(NOT EXISTS "${reuse_toml_path}" AND NOT QT_FORCE_REUSE_LINT_ERROR)
        set(arg_NO_ERROR TRUE)
    endif()

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"Running 'reuse lint' failed: \${res}\")
            endif()
")
    endif()

    set(content "
        message(STATUS \"Running 'reuse lint' in '${PROJECT_SOURCE_DIR}'.\")
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_REUSE} --root \"${PROJECT_SOURCE_DIR}\" lint
            RESULT_VARIABLE res
        )
        ${handle_error}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_op_build "${sbom_dir}/run_reuse_lint_build.cmake")
    file(GENERATE OUTPUT "${file_op_build}" CONTENT "${content}")

    # Allow skipping running 'reuse lint' during installation. But still allow running it during
    # build time. This is a fail safe opt-out in case some repo needs it.
    if(QT_FORCE_SKIP_REUSE_LINT_ON_INSTALL)
        set(skip_message "Skipping running 'reuse lint' in '${PROJECT_SOURCE_DIR}'.")

        set(content "
            message(STATUS \"${skip_message}\")
")
        set(file_op_install "${sbom_dir}/run_reuse_lint_install.cmake")
        file(GENERATE OUTPUT "${file_op_install}" CONTENT "${content}")
    else()
        # Just reuse the already generated script for installation as well.
        set(file_op_install "${file_op_build}")
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${file_op_install}")

    if(arg_BUILD_TIME_SCRIPT_PATH_OUT_VAR)
        set(${arg_BUILD_TIME_SCRIPT_PATH_OUT_VAR} "${file_op_build}" PARENT_SCOPE)
    endif()
endfunction()
