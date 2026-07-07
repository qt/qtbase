# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


# The metatypes artifacts that qt_extract_metatypes(lib1) creates.
set(json_list_file "${RunCMake_TEST_BINARY_DIR}/meta_types/lib1_json_file_list.txt")
set(metatypes_file "${RunCMake_TEST_BINARY_DIR}/meta_types/qt6lib1_metatypes.json")

# Assert that lib1's metatypes json file list has exactly EXPECTED_ENTRY_COUNT
# entries, that every listed file exists, and that the collected metatypes json
# mentions each class name in EXPECTED_CLASS_NAMES.
function(check_metatypes_json_list)
    set(opt_args "")
    set(single_args
        EXPECTED_ENTRY_COUNT
    )
    set(multi_args
        EXPECTED_CLASS_NAMES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(failure "")

    if(NOT EXISTS "${json_list_file}")
        string(APPEND failure
            "Metatypes json file list not found:\n  ${json_list_file}\n")
    else()
        file(STRINGS "${json_list_file}" entries)
        list(LENGTH entries count)
        if(NOT count EQUAL arg_EXPECTED_ENTRY_COUNT)
            string(APPEND failure
                "Expected ${arg_EXPECTED_ENTRY_COUNT} entries in ${json_list_file}, got "
                "${count}:\n  ${entries}\n")
        endif()
        foreach(entry IN LISTS entries)
            if(NOT EXISTS "${entry}")
                string(APPEND failure
                    "Metatypes json list references a non-existent file:\n  ${entry}\n")
            endif()
        endforeach()
    endif()

    if(NOT EXISTS "${metatypes_file}")
        string(APPEND failure
            "Metatypes json file not found:\n  ${metatypes_file}\n")
    else()
        file(READ "${metatypes_file}" metatypes)
        foreach(class_name IN LISTS arg_EXPECTED_CLASS_NAMES)
            if(NOT metatypes MATCHES "\"className\": \"${class_name}\"")
                string(APPEND failure
                    "Metatypes json is missing class '${class_name}':\n"
                    "${metatypes}\n")
            endif()
        endforeach()
    endif()

    if(NOT failure STREQUAL "")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}${failure}" PARENT_SCOPE)
    endif()
endfunction()

# Assert that lib1's metatypes json file list contains an entry matching each
# regex in EXPECTED_PATTERNS.
function(check_json_list_entries)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        EXPECTED_PATTERNS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(failure "")

    if(NOT EXISTS "${json_list_file}")
        string(APPEND failure
            "Metatypes json file list not found:\n  ${json_list_file}\n")
    else()
        file(READ "${json_list_file}" all_entries)
        foreach(pattern IN LISTS arg_EXPECTED_PATTERNS)
            if(NOT all_entries MATCHES "${pattern}")
                string(APPEND failure
                    "Metatypes json list has no entry matching '${pattern}':\n"
                    "${all_entries}\n")
            endif()
        endforeach()
    endif()

    if(NOT failure STREQUAL "")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}${failure}" PARENT_SCOPE)
    endif()
endfunction()
