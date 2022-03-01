function(qt_print_feature_summary)
    include(FeatureSummary)
    # Show which packages were found.
    feature_summary(INCLUDE_QUIET_PACKAGES
                    WHAT PACKAGES_FOUND
                         REQUIRED_PACKAGES_NOT_FOUND
                         RECOMMENDED_PACKAGES_NOT_FOUND
                         OPTIONAL_PACKAGES_NOT_FOUND
                         RUNTIME_PACKAGES_NOT_FOUND
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)
    qt_configure_print_summary()
endfunction()

function(qt_print_build_instructions)
    if((NOT PROJECT_NAME STREQUAL "QtBase" AND
        NOT PROJECT_NAME STREQUAL "Qt") OR
       QT_BUILD_STANDALONE_TESTS)

        return()
    endif()

    set(build_command "cmake --build . --parallel")
    set(install_command "cmake --install .")

    # Suggest "ninja install" for Multi-Config builds
    # until https://gitlab.kitware.com/cmake/cmake/-/issues/21475 is fixed.
    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
        set(install_command "ninja install")
    endif()

    set(configure_module_command "qt-configure-module")
    if(CMAKE_HOST_WIN32)
        string(APPEND configure_module_command ".bat")
    endif()
    if("${CMAKE_STAGING_PREFIX}" STREQUAL "")
        set(local_install_prefix "${CMAKE_INSTALL_PREFIX}")
    else()
        set(local_install_prefix "${CMAKE_STAGING_PREFIX}")
    endif()

    set(msg "")

    list(APPEND msg "Qt is now configured for building. Just run '${build_command}'\n")
    if(QT_WILL_INSTALL)
        list(APPEND msg "Once everything is built, you must run '${install_command}'")
        list(APPEND msg "Qt will be installed into '${CMAKE_INSTALL_PREFIX}'")
    else()
        list(APPEND msg
            "Once everything is built, Qt is installed. You should NOT run '${install_command}'")
        list(APPEND msg
            "Note that this build cannot be deployed to other machines or devices.")
    endif()
    list(APPEND msg
        "\nTo configure and build other Qt modules, you can use the following convenience script:
        ${local_install_prefix}/${INSTALL_BINDIR}/${configure_module_command}")
    list(APPEND msg "\nIf reconfiguration fails for some reason, try removing 'CMakeCache.txt' \
from the build directory \n")
    list(JOIN msg "\n" msg)

    if(NOT QT_INTERNAL_BUILD_INSTRUCTIONS_SHOWN)
        message(STATUS "${msg}")
    endif()

    set(QT_INTERNAL_BUILD_INSTRUCTIONS_SHOWN "TRUE" CACHE STRING "" FORCE)
endfunction()

function(qt_configure_print_summary)
    # Evaluate all recorded commands.
    qt_configure_eval_commands()

    set(summary_file "${CMAKE_BINARY_DIR}/config.summary")
    file(WRITE "${summary_file}" "")
    # Show Qt-specific configure summary and any notes, wranings, etc.
    if(__qt_configure_reports)
        message(STATUS "Configure summary:\n${__qt_configure_reports}")
        file(APPEND "${summary_file}" "${__qt_configure_reports}")
    endif()
    if(__qt_configure_notes)
        message("${__qt_configure_notes}")
        file(APPEND "${summary_file}" "${__qt_configure_notes}")
    endif()
    if(__qt_configure_warnings)
        message("${__qt_configure_warnings}")
        file(APPEND "${summary_file}" "${__qt_configure_warnings}")
    endif()
    if(__qt_configure_errors)
        message("${__qt_configure_errors}")
        file(APPEND "${summary_file}" "${__qt_configure_errors}")
    endif()
    message("")
    if(__qt_configure_an_error_occurred)
        message(FATAL_ERROR "Check the configuration messages for an error that has occurred.")
    endif()
    file(APPEND "${summary_file}" "\n")
endfunction()

# Takes a list of arguments, and saves them to be evaluated at the end of the configuration
# phase when the configuration summary is shown.
#
# RECORD_ON_FEATURE_EVALUATION option allows to record the command even while the feature
# evaluation-only stage.
function(qt_configure_record_command)
    cmake_parse_arguments(arg "RECORD_ON_FEATURE_EVALUATION"
                          ""
                          "" ${ARGV})
    # Don't record commands when only evaluating features of a configure.cmake file.
    if(__QtFeature_only_evaluate_features AND NOT arg_RECORD_ON_FEATURE_EVALUATION)
        return()
    endif()

    get_property(command_count GLOBAL PROPERTY qt_configure_command_count)

    if(NOT DEFINED command_count)
        set(command_count 0)
    endif()

    set_property(GLOBAL PROPERTY qt_configure_command_${command_count} "${arg_UNPARSED_ARGUMENTS}")

    math(EXPR command_count "${command_count}+1")
    set_property(GLOBAL PROPERTY qt_configure_command_count "${command_count}")
endfunction()

function(qt_configure_eval_commands)
    get_property(command_count GLOBAL PROPERTY qt_configure_command_count)
    if(NOT command_count)
        set(command_count 0)
    endif()
    set(command_index 0)

    while(command_index LESS command_count)
        get_property(command_args GLOBAL PROPERTY qt_configure_command_${command_index})
        if(NOT command_args)
            message(FATAL_ERROR
                "Empty arguments encountered while processing qt configure reports.")
        endif()

        list(POP_FRONT command_args command_name)
        if(command_name STREQUAL ADD_SUMMARY_SECTION)
            qt_configure_process_add_summary_section(${command_args})
        elseif(command_name STREQUAL END_SUMMARY_SECTION)
            qt_configure_process_end_summary_section(${command_args})
        elseif(command_name STREQUAL ADD_REPORT_ENTRY)
            qt_configure_process_add_report_entry(${command_args})
        elseif(command_name STREQUAL ADD_SUMMARY_ENTRY)
            qt_configure_process_add_summary_entry(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_TYPE_AND_CONFIG)
            qt_configure_process_add_summary_build_type_and_config(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_MODE)
            qt_configure_process_add_summary_build_mode(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_PARTS)
            qt_configure_process_add_summary_build_parts(${command_args})
        endif()

        math(EXPR command_index "${command_index}+1")
    endwhile()

    # Propagate content to parent.
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
    set(__qt_configure_notes "${__qt_configure_notes}" PARENT_SCOPE)
    set(__qt_configure_warnings "${__qt_configure_warnings}" PARENT_SCOPE)
    set(__qt_configure_errors "${__qt_configure_errors}" PARENT_SCOPE)
    set(__qt_configure_an_error_occurred "${__qt_configure_an_error_occurred}" PARENT_SCOPE)
endfunction()

macro(qt_configure_add_report message)
    string(APPEND __qt_configure_reports "\n${message}")
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
endmacro()

macro(qt_configure_add_report_padded label message)
    qt_configure_get_padded_string("${__qt_configure_indent}${label}" "${message}" padded_message)
    string(APPEND __qt_configure_reports "\n${padded_message}")
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
endmacro()

# Pad 'label' and 'value' with dots like this:
# "label ............... value"
#
# PADDING_LENGTH specifies the number of characters from the start to the last dot.
#                Default is 30.
# MIN_PADDING    specifies the minimum number of dots that are used for the padding.
#                Default is 0.
function(qt_configure_get_padded_string label value out_var)
    cmake_parse_arguments(arg "" "PADDING_LENGTH;MIN_PADDING" "" ${ARGN})
    if("${arg_MIN_PADDING}" STREQUAL "")
        set(arg_MIN_PADDING 0)
    endif()
    if(arg_PADDING_LENGTH)
        set(pad_string "")
        math(EXPR n "${arg_PADDING_LENGTH} - 1")
        foreach(i RANGE ${n})
            string(APPEND pad_string ".")
        endforeach()
    else()
        set(pad_string ".........................................")
    endif()
    string(LENGTH "${label}" label_len)
    string(LENGTH "${pad_string}" pad_len)
    math(EXPR pad_len "${pad_len}-${label_len}")
    if(pad_len LESS "0")
        set(pad_len ${arg_MIN_PADDING})
    endif()
    string(SUBSTRING "${pad_string}" 0 "${pad_len}" pad_string)
    set(output "${label} ${pad_string} ${value}")
    set("${out_var}" "${output}" PARENT_SCOPE)
endfunction()

function(qt_configure_add_summary_entry)
    qt_configure_record_command(ADD_SUMMARY_ENTRY ${ARGV})
endfunction()

function(qt_configure_process_add_summary_entry)
    qt_parse_all_arguments(arg "qt_configure_add_summary_entry"
        ""
        "ARGS;TYPE;MESSAGE" "CONDITION" ${ARGN})

    if(NOT arg_TYPE)
        set(arg_TYPE "feature")
    endif()

    if(NOT "${arg_CONDITION}" STREQUAL "")
        qt_evaluate_config_expression(condition_result ${arg_CONDITION})
        if(NOT condition_result)
            return()
        endif()
    endif()

    if(arg_TYPE STREQUAL "firstAvailableFeature")
        set(first_feature_found FALSE)
        set(message "")
        string(REPLACE " " ";" args_list "${arg_ARGS}")
        foreach(feature ${args_list})
            qt_feature_normalize_name("${feature}" feature)
            if(NOT DEFINED QT_FEATURE_${feature})
                message(FATAL_ERROR "Asking for a report on undefined feature ${feature}.")
            endif()
            if(QT_FEATURE_${feature})
                set(first_feature_found TRUE)
                set(message "${QT_FEATURE_LABEL_${feature}}")
                break()
            endif()
        endforeach()

        if(NOT first_feature_found)
            set(message "<none>")
        endif()
        qt_configure_add_report_padded("${arg_MESSAGE}" "${message}")
    elseif(arg_TYPE STREQUAL "featureList")
        set(available_features "")
        string(REPLACE " " ";" args_list "${arg_ARGS}")

        foreach(feature ${args_list})
            qt_feature_normalize_name("${feature}" feature)
            if(NOT DEFINED QT_FEATURE_${feature})
                message(FATAL_ERROR "Asking for a report on undefined feature ${feature}.")
            endif()
            if(QT_FEATURE_${feature})
                list(APPEND available_features "${QT_FEATURE_LABEL_${feature}}")
            endif()
        endforeach()

        if(NOT available_features)
            set(message "<none>")
        else()
            list(JOIN available_features " " message)
        endif()
        qt_configure_add_report_padded("${arg_MESSAGE}" "${message}")
    elseif(arg_TYPE STREQUAL "feature")
        qt_feature_normalize_name("${arg_ARGS}" feature)

        set(label "${QT_FEATURE_LABEL_${feature}}")

        if(NOT label)
            set(label "${feature}")
        endif()

        if(QT_FEATURE_${feature})
            set(value "yes")
        else()
            set(value "no")
        endif()

        qt_configure_add_report_padded("${label}" "${value}")
    elseif(arg_TYPE STREQUAL "message")
        qt_configure_add_report_padded("${arg_ARGS}" "${arg_MESSAGE}")
    endif()
endfunction()

function(qt_configure_add_summary_build_type_and_config)
    qt_configure_record_command(ADD_BUILD_TYPE_AND_CONFIG ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_type_and_config)
    get_property(subarch_summary GLOBAL PROPERTY qt_configure_subarch_summary)
    if(APPLE AND (CMAKE_OSX_ARCHITECTURES MATCHES ";"))
        set(message
            "Building for: ${QT_QMAKE_TARGET_MKSPEC} (${CMAKE_OSX_ARCHITECTURES}), ${TEST_architecture_arch} features: ${subarch_summary})")
    else()
        set(message
            "Building for: ${QT_QMAKE_TARGET_MKSPEC} (${TEST_architecture_arch}, CPU features: ${subarch_summary})")
    endif()
    qt_configure_add_report("${message}")

    set(message "Compiler: ")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        string(APPEND message "clang (Apple)")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        string(APPEND message "clang (Intel LLVM)")
    elseif(CLANG)
        string(APPEND message "clang")
    elseif(QCC)
        string(APPEND message "rim_qcc")
    elseif(GCC)
        string(APPEND message "gcc")
    elseif(MSVC)
        string(APPEND message "msvc")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GHS")
        string(APPEND message "ghs")
    else()
        string(APPEND message "unknown (${CMAKE_CXX_COMPILER_ID})")
    endif()
    string(APPEND message " ${CMAKE_CXX_COMPILER_VERSION}")
    qt_configure_add_report("${message}")
endfunction()

function(qt_configure_add_summary_build_mode)
    qt_configure_record_command(ADD_BUILD_MODE ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_mode label)
    set(message "")
    if(DEFINED CMAKE_BUILD_TYPE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(APPEND message "debug")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            string(APPEND message "release")
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            string(APPEND message "release (with debug info)")
        else()
            string(APPEND message "${CMAKE_BUILD_TYPE}")
        endif()
    elseif(DEFINED CMAKE_CONFIGURATION_TYPES)
        if("Release" IN_LIST CMAKE_CONFIGURATION_TYPES
                AND "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
            string(APPEND message "debug and release")
        elseif("RelWithDebInfo" IN_LIST CMAKE_CONFIGURATION_TYPES
                AND "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
            string(APPEND message "debug and release (with debug info)")
        else()
            string(APPEND message "${CMAKE_CONFIGURATION_TYPES}")
        endif()
    endif()

    qt_configure_add_report_padded("${label}" "${message}")
endfunction()

function(qt_configure_add_summary_build_parts)
    qt_configure_record_command(ADD_BUILD_PARTS ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_parts label)
    qt_get_build_parts(parts)
    string(REPLACE ";" " " message "${parts}")
    qt_configure_add_report_padded("${label}" "${message}")
endfunction()

function(qt_configure_add_summary_section)
    qt_configure_record_command(ADD_SUMMARY_SECTION ${ARGV})
endfunction()

function(qt_configure_process_add_summary_section)
    qt_parse_all_arguments(arg "qt_configure_add_summary_section"
        "" "NAME" "" ${ARGN})

    qt_configure_add_report("${__qt_configure_indent}${arg_NAME}:")
    if(NOT DEFINED __qt_configure_indent)
        set(__qt_configure_indent "  " PARENT_SCOPE)
    else()
        set(__qt_configure_indent "${__qt_configure_indent}  " PARENT_SCOPE)
    endif()
endfunction()

function(qt_configure_end_summary_section)
    qt_configure_record_command(END_SUMMARY_SECTION ${ARGV})
endfunction()

function(qt_configure_process_end_summary_section)
    string(LENGTH "${__qt_configure_indent}" indent_len)
    if(indent_len GREATER_EQUAL 2)
        string(SUBSTRING "${__qt_configure_indent}" 2 -1 __qt_configure_indent)
        set(__qt_configure_indent "${__qt_configure_indent}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_configure_add_report_entry)
    qt_configure_record_command(ADD_REPORT_ENTRY ${ARGV})
endfunction()

function(qt_configure_add_report_error error)
    message(SEND_ERROR "${error}")
    qt_configure_add_report_entry(TYPE ERROR MESSAGE "${error}" CONDITION TRUE ${ARGN})
endfunction()

function(qt_configure_process_add_report_entry)
    qt_parse_all_arguments(arg "qt_configure_add_report_entry"
        ""
        "TYPE;MESSAGE" "CONDITION" ${ARGN})

    set(possible_types NOTE WARNING ERROR FATAL_ERROR)
    if(NOT "${arg_TYPE}" IN_LIST possible_types)
        message(FATAL_ERROR "qt_configure_add_report_entry: '${arg_TYPE}' is not a valid type.")
    endif()

    if(NOT arg_MESSAGE)
        message(FATAL_ERROR "qt_configure_add_report_entry: Empty message given.")
    endif()

    if(arg_TYPE STREQUAL "NOTE")
        set(contents_var "__qt_configure_notes")
        set(prefix "Note: ")
    elseif(arg_TYPE STREQUAL "WARNING")
        set(contents_var "__qt_configure_warnings")
        set(prefix "WARNING: ")
    elseif(arg_TYPE STREQUAL "ERROR")
        set(contents_var "__qt_configure_errors")
        set(prefix "ERROR: ")
    elseif(arg_TYPE STREQUAL "FATAL_ERROR")
        set(contents_var "__qt_configure_errors")
        set(prefix "FATAL ERROR: ")
    endif()

    if(NOT "${arg_CONDITION}" STREQUAL "")
        qt_evaluate_config_expression(condition_result ${arg_CONDITION})
    endif()

    if("${arg_CONDITION}" STREQUAL "" OR condition_result)
        set(new_report "${prefix}${arg_MESSAGE}")
        string(APPEND "${contents_var}" "\n${new_report}")

        if(arg_TYPE STREQUAL "ERROR" OR arg_TYPE STREQUAL "FATAL_ERROR")
            set(__qt_configure_an_error_occurred "TRUE" PARENT_SCOPE)
        endif()
    endif()

    set("${contents_var}" "${${contents_var}}" PARENT_SCOPE)
endfunction()
