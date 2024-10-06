# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(generate_hash_folder target_name infile out_folder)
    get_filename_component(infile_abs "${infile}" ABSOLUTE)
    string(SHA1 infile_hash "${target_name}${infile_abs}")
    string(SUBSTRING "${infile_hash}" 0 6 short_hash)
    set(${out_folder} "${short_hash}" PARENT_SCOPE)
endfunction()

function(get_latest_vs_generator output)
    execute_process(COMMAND ${CMAKE_COMMAND} -G
        ERROR_VARIABLE CMAKE_GENERATORS_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCHALL "Visual Studio [0-9]+ [0-9]+" vs_generators
        "${CMAKE_GENERATORS_ERROR}")

    if(NOT vs_generators)
        message(FATAL_ERROR "No visual studio generators found")
    endif()

    set(last_version "0")
    set(last_generator "")
    foreach(generator IN LISTS vs_generators)
        string(REGEX MATCH "Visual Studio ([0-9]+) [0-9]+" unused "${generator}")
        if("${CMAKE_MATCH_1}" VERSION_GREATER "${last_version}")
            set(last_version "${CMAKE_MATCH_1}")
            set(last_generator "${CMAKE_MATCH_0}")
        endif()
    endforeach()
    set(${output} "${last_generator}" PARENT_SCOPE)
endfunction()

function(check_unwanted_builds_after_first_build cmake_output test_name test_dir
    generator)
    set(unwanted_builds_success_message
        "\"${test_name}\" in \"${test_dir}\" -> No unwanted builds")
    set(unwanted_builds_failure_message
        "\"${test_name}\" in \"${test_dir}\" -> Unwanted builds found")
    if(${generator} MATCHES "Ninja")
        expect_string_not_contains(${cmake_output}
        "widget2.cpp.o.d|mainwindow.cpp.o.d"
         SUCCESS_MESSAGE ${unwanted_builds_success_message}
         FAILURE_MESSAGE ${unwanted_builds_failure_message})
    elseif(${generator} MATCHES "Make")
        string(CONCAT not_expect_string
            "Building CXX object UicIncrementalBuild/CMakeFiles"
            "/example.dir/src/widget2.cpp.o|Building CXX object UicIncremental"
            "Build/CMakeFiles/example.dir/src/mainwindow.cpp.o")
        expect_string_not_contains(${cmake_output} "${not_expect_string}"
            SUCCESS_MESSAGE ${unwanted_builds_success_message}
            FAILURE_MESSAGE ${unwanted_builds_failure_message})
    elseif(${generator} MATCHES "Visual Studio")
        expect_string_not_contains(${cmake_output} "widget2.cpp|mainwindow.cpp"
            SUCCESS_MESSAGE ${unwanted_builds_success_message}
            FAILURE_MESSAGE ${unwanted_builds_failure_message})
    elseif(${generator} MATCHES "Xcode")
        expect_string_not_contains(${cmake_output} "widget2.cpp|mainwindow.cpp"
            SUCCESS_MESSAGE ${unwanted_builds_success_message}
            FAILURE_MESSAGE ${unwanted_builds_failure_message})
    endif()
endfunction()

function(check_output_after_second_build cmake_output test_name
    test_dir generator)
    set(second_build_success_message
        "\"${test_name}\" in \"${test_dir}\" -> Generation of UI files were not \
triggered in the second build")
    set(second_build_failure_message
        "\"${test_name}\" in \"${test_dir}\" -> Generation of UI files were \
triggered in the second build")

    if(${generator} MATCHES "Ninja")
        expect_string_contains(${cmake_output} "ninja: no work to do."
                               SUCCESS_MESSAGE ${second_build_success_message}
                               FAILURE_MESSAGE ${second_build_failure_message})
    elseif(${generator} MATCHES "Visual Studio" OR ${generator} MATCHES "Xcode")
        expect_string_not_contains(${cmake_output} "mainwindow"
                                  SUCCESS_MESSAGE
                                  ${second_build_success_message}
                                  FAILURE_MESSAGE
                                  ${second_build_failure_message})
    elseif(${generator} MATCHES "Makefiles")
        expect_string_not_contains(${cmake_output} "mainwindow.cpp.o.d -o"
                               SUCCESS_MESSAGE ${second_build_success_message}
                               FAILURE_MESSAGE ${second_build_failure_message})
    endif()
endfunction()

function(incremental_build_test)
    set(options CHECK_UNWANTED_BUILDS)
    set(oneValueArgs CONFIG TEST_NAME SOURCE_DIR BUILD_DIR FILE_TO_TOUCH
        FILE_TO_CHECK FOLDER_TO_CHECK GENERATOR)
    set(multiValueArgs ADDITIONAL_ARGS)

    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN})

    if ("${arg_SOURCE_DIR}" STREQUAL "")
        message(FATAL_ERROR "FAIL: \"${arg_TEST_NAME}\" test failed because "
            "SOURCE_DIR is empty")
    endif()

    if ("${arg_BUILD_DIR}" STREQUAL "")
        message(FATAL_ERROR "FAIL: \"${arg_TEST_NAME}\" test failed because "
            "BUILD_DIR is empty")
    endif()

    if ("${arg_GENERATOR}" STREQUAL "")
        message(FATAL_ERROR "FAIL: \"${arg_TEST_NAME}\" test failed because "
            "GENERATOR is empty")
    endif()

    run_cmake_configure(SOURCE_DIR "${arg_SOURCE_DIR}"
                        BUILD_DIR "${arg_BUILD_DIR}"
                        GENERATOR "${arg_GENERATOR}"
                        CLEAN_FIRST
                        ADDITIONAL_ARGS ${arg_ADDITIONAL_ARGS}
                        OUTPUT_VARIABLE cmake_output
                        ERROR_VARIABLE cmake_error
                        RESULT_VARIABLE cmake_result)

    if(${cmake_result} EQUAL 0)
        message(STATUS
            "PASS: \"${arg_TEST_NAME}\" test in ${arg_BUILD_DIR} was configured "
            "successfully")
    else()
        message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error: "
            "${cmake_error}\nFAIL: \"${arg_TEST_NAME}\" test in ${arg_BUILD_DIR}"
            " failed to configure")
    endif()

    if(NOT "${arg_CONFIG}" STREQUAL "single_config")
        set(config_arg "${arg_CONFIG}")
    endif()

    run_cmake_build(BUILD_DIR ${arg_BUILD_DIR}
                    VERBOSE ON
                    CONFIG ${config_arg}
                    OUTPUT_VARIABLE cmake_build_output
                    ERROR_VARIABLE cmake_build_error
                    RESULT_VARIABLE cmake_build_result)

    if(${cmake_build_result} EQUAL 0)
        message(STATUS
            "PASS: \"${arg_TEST_NAME}\" test in ${arg_BUILD_DIR} was built "
            "successfully")
    else()
        message(FATAL_ERROR
            "cmake_build_output: ${cmake_build_output}\ncmake_build_error: "
            "${cmake_build_error}\nFAIL: \"${arg_TEST_NAME}\" test in "
            "${arg_BUILD_DIR} failed to build")
    endif()

    if(NOT "${arg_FILE_TO_CHECK}" STREQUAL "")
        if(NOT EXISTS "${arg_FILE_TO_CHECK}")
            message(FATAL_ERROR "FAIL: \"${arg_TEST_NAME}\" ${arg_FILE_TO_CHECK}"
                " could not be found")
        else()
            message(STATUS "PASS: \"${arg_TEST_NAME}\" \"${arg_FILE_TO_CHECK}\" "
                "was generated successfully")
        endif()
    endif()

    if(NOT "${arg_FOLDER_TO_CHECK}" STREQUAL "" AND NOT WIN32)
        if(NOT EXISTS "${arg_FOLDER_TO_CHECK}")
            message(FATAL_ERROR
                "FAIL: \"${arg_TEST_NAME}\" Folder \"${arg_FOLDER_TO_CHECK}\" "
                "does not exist")
        else()
            message(STATUS
                "PASS: \"${arg_TEST_NAME}\" Folder \"${arg_FOLDER_TO_CHECK}\" "
                "exists")
        endif()
    endif()

    if(NOT "${arg_FILE_TO_TOUCH}" STREQUAL "")
        file(TOUCH "${arg_FILE_TO_TOUCH}")
    endif()

    run_cmake_build(BUILD_DIR ${arg_BUILD_DIR}
                    VERBOSE ON
                    CONFIG ${arg_CONFIG}
                    OUTPUT_VARIABLE cmake_build_output)
    if(${arg_CHECK_UNWANTED_BUILDS})
        check_unwanted_builds_after_first_build(${cmake_build_output}
            ${arg_TEST_NAME} ${arg_BUILD_DIR} ${arg_GENERATOR})
    endif()

    run_cmake_build(BUILD_DIR ${arg_BUILD_DIR}
                    VERBOSE ON
                    CONFIG ${arg_CONFIG}
                    OUTPUT_VARIABLE cmake_build_output)
    check_output_after_second_build(${cmake_build_output}
        ${arg_TEST_NAME} ${arg_BUILD_DIR} ${arg_GENERATOR})
endfunction()

function(get_generators output)
    if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux|GNU")
        set(generators "Unix Makefiles" "Ninja" "Ninja Multi-Config")
    elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        # CI fails with Clang and Visual Studio generators.
        # That's why discard that combination.
        if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND
            NOT MINGW)
            get_latest_vs_generator(latest_vs)
        endif()
        set(generators "Ninja" "Ninja Multi-Config" "${latest_vs}")
    elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
        # TODO: Add Xcode generator when
        # https://gitlab.kitware.com/cmake/cmake/-/issues/25790 is fixed.
        # Otherwise, adding Xcode generator might fail CI due to the timeout
        # issue.
        set(generators "Unix Makefiles" "Ninja" "Ninja Multi-Config")
    else()
        string(JOIN "" ERROR_MESSAGE
            "FAIL: host OS not supported for this test."
            "host : ${CMAKE_HOST_SYSTEM_NAME}")
        message(FATAL_ERROR "${ERROR_MESSAGE}")
    endif()
    set(${output} "${generators}" PARENT_SCOPE)
endfunction()
