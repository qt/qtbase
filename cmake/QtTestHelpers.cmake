# Simple wrapper around qt_internal_add_executable for benchmarks which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_internal_add_executable() for more details.
function(qt_internal_add_benchmark target)

    qt_parse_all_arguments(arg "qt_add_benchmark"
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_internal_add_executable_optional_args}"
            "${__qt_internal_add_executable_single_args}"
            "${__qt_internal_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_internal_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

    qt_internal_collect_command_environment(benchmark_env_path benchmark_env_plugin_path)

    # Add a ${target}_benchmark generator target, to run single benchmark more easily.
    set(benchmark_wrapper_file "${arg_OUTPUT_DIRECTORY}/${target}Wrapper$<CONFIG>.cmake")
    qt_internal_create_command_script(COMMAND "$<TARGET_FILE:${target}>"
                                      OUTPUT_FILE "${benchmark_wrapper_file}"
                                      ENVIRONMENT "PATH" "${benchmark_env_path}"
                                                  "QT_PLUGIN_PATH" "${benchmark_env_plugin_path}"
    )

    add_custom_target("${target}_benchmark"
        VERBATIM
        COMMENT "Running benchmark ${target}"
        COMMAND "${CMAKE_COMMAND}" "-P" "${benchmark_wrapper_file}"
    )

    add_dependencies("${target}_benchmark" "${target}")

    # Add benchmark to meta target if it exists.
    if (TARGET benchmark)
        add_dependencies("benchmark" "${target}_benchmark")
    endif()
endfunction()

# Simple wrapper around qt_internal_add_executable for manual tests which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_internal_add_executable() for more details.
function(qt_internal_add_manual_test target)

    qt_parse_all_arguments(arg "qt_add_manual_test"
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_internal_add_executable_optional_args}"
            "${__qt_internal_add_executable_single_args}"
            "${__qt_internal_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_internal_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

endfunction()

# This function will configure the fixture for the network tests that require docker network services
# qmake counterpart: qtbase/mkspecs/features/unsupported/testserver.prf
function(qt_internal_setup_docker_test_fixture name)
    # Only Linux is provisioned with docker at this time
    if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
      return()
    endif()

    set(QT_TEST_SERVER_LIST ${ARGN})
    set(DNSDOMAIN test-net.qt.local)

    find_program(QT_DOCKER_COMPOSE docker-compose)
    if (NOT QT_DOCKER_COMPOSE)
        message(WARNING "docker-compose was not found. Docker network tests will not be run.")
        return()
    endif()
    if (NOT DEFINED QT_DOCKER_COMPOSE_VERSION)
      execute_process(COMMAND "${QT_DOCKER_COMPOSE}" --version OUTPUT_VARIABLE QT_DOCKER_COMPOSE_VERSION)
      string(REPLACE "\n" "" QT_DOCKER_COMPOSE_VERSION "${QT_DOCKER_COMPOSE_VERSION}")
      set(QT_DOCKER_COMPOSE_VERSION "${QT_DOCKER_COMPOSE_VERSION}" CACHE STRING "docker compose version")
    endif()

    find_program(QT_DOCKER docker)
    if (NOT QT_DOCKER)
        message(WARNING "docker was not found. Docker network tests will not be run.")
        return()
    endif()
    if (NOT DEFINED QT_DOCKER_TEST_SERVER)
        execute_process(COMMAND "${QT_DOCKER}" images -aq "qt-test-server-*" OUTPUT_VARIABLE QT_DOCKER_TEST_SERVER)
        if (NOT QT_DOCKER_TEST_SERVER)
            message(WARNING
                "Docker image qt-test-server-* not found.\n"
                "Run the provisioning script (coin/provisioning/.../testserver/docker_testserver.sh) in advance\n"
                "Docker network tests will not be run.")
            return()
        endif()
        set(QT_DOCKER_TEST_SERVER "ON" CACHE BOOL "docker qt-test-server-* present")
    endif()

    target_compile_definitions("${name}"
        PRIVATE
            QT_TEST_SERVER QT_TEST_SERVER_NAME QT_TEST_SERVER_DOMAIN=\"${DNSDOMAIN}\"
    )

    if(DEFINED QT_TESTSERVER_COMPOSE_FILE)
        set(TESTSERVER_COMPOSE_FILE ${QT_TESTSERVER_COMPOSE_FILE})
    else()
        set(TESTSERVER_COMPOSE_FILE "${QT_SOURCE_TREE}/tests/testserver/docker-compose-bridge-network.yml")
    endif()

    # Bring up test servers and make sure the services are ready.
    add_test(NAME ${name}-setup COMMAND
        "${QT_DOCKER_COMPOSE}" -f ${TESTSERVER_COMPOSE_FILE} up --build -d --force-recreate --timeout 1 ${QT_TEST_SERVER_LIST}
    )
    # Stop and remove test servers after testing.
    add_test(NAME ${name}-cleanup COMMAND
        "${QT_DOCKER_COMPOSE}" -f ${TESTSERVER_COMPOSE_FILE} down --timeout 1
    )

    set_tests_properties(${name}-setup PROPERTIES FIXTURES_SETUP ${name}-docker)
    set_tests_properties(${name}-cleanup PROPERTIES FIXTURES_CLEANUP ${name}-docker)
    set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED ${name}-docker)

    foreach(test_name ${name} ${name}-setup ${name}-cleanup)
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT "testserver=${QT_DOCKER_COMPOSE_VERSION}")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT TEST_DOMAIN=${DNSDOMAIN})
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT "SHARED_DATA=${QT_MKSPECS_DIR}/features/data/testserver")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT SHARED_SERVICE=bridge-network)
    endforeach()

endfunction()

# This function creates a CMake test target with the specified name for use with CTest.
#
# All tests are wrapped with cmake script that supports TESTARGS and TESTRUNNER environment
# variables handling. Endpoint wrapper may be used standalone as cmake script to run tests e.g.:
# TESTARGS="-o result.xml,xunitxml" TESTRUNNER="testrunner --arg" ./tst_simpleTestWrapper.cmake
# On non-UNIX machine you may need to use 'cmake -P' explicitly to execute wrapper.
# You may avoid test wrapping by either passing NO_WRAPPER option or switching QT_NO_TEST_WRAPPERS
# to ON. This is helpful if you want to use internal CMake tools within tests, like memory or
# sanitizer checks. See https://cmake.org/cmake/help/v3.19/manual/ctest.1.html#ctest-memcheck-step
function(qt_internal_add_test name)
    # EXCEPTIONS is a noop as they are enabled by default.
    qt_parse_all_arguments(arg "qt_add_test"
        "RUN_SERIAL;EXCEPTIONS;NO_EXCEPTIONS;GUI;QMLTEST;CATCH;LOWDPI;NO_WRAPPER"
        "OUTPUT_DIRECTORY;WORKING_DIRECTORY;TIMEOUT;VERSION"
        "QML_IMPORTPATH;TESTDATA;QT_TEST_SERVER_LIST;${__default_private_args};${__default_public_args}" ${ARGN}
    )

    if (NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # Qt modules get compiled without exceptions enabled by default.
    # However, testcases should be still built with exceptions.
    set(exceptions_text "EXCEPTIONS")
    if (${arg_NO_EXCEPTIONS})
        set(exceptions_text "")
    endif()

    if (${arg_GUI})
        set(gui_text "GUI")
    endif()

    if (arg_VERSION)
        set(version_arg VERSION "${arg_VERSION}")
    endif()

    # Handle cases where we have a qml test without source files
    if (arg_SOURCES)
        set(private_includes
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<BUILD_INTERFACE:${QT_BUILD_DIR}/include>"
             ${arg_INCLUDE_DIRECTORIES}
        )

        qt_internal_add_executable("${name}"
            ${exceptions_text}
            ${gui_text}
            ${version_arg}
            NO_INSTALL
            OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            SOURCES "${arg_SOURCES}"
            INCLUDE_DIRECTORIES
                ${private_includes}
            DEFINES
                QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}"
                QT_TESTCASE_SOURCEDIR="${CMAKE_CURRENT_SOURCE_DIR}"
                ${arg_DEFINES}
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Core ${QT_CMAKE_EXPORT_NAMESPACE}::Test ${arg_PUBLIC_LIBRARIES}
            LIBRARIES ${arg_LIBRARIES}
            COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
            LINK_OPTIONS ${arg_LINK_OPTIONS}
            MOC_OPTIONS ${arg_MOC_OPTIONS}
            ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
            DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        )

        # Tests should not be bundles on macOS even if arg_GUI is true, because some tests make
        # assumptions about the location of helper processes, and those paths would be different
        # if a test is built as a bundle.
        set_property(TARGET "${name}" PROPERTY MACOSX_BUNDLE FALSE)
        # The same goes for WIN32_EXECUTABLE, but because it will detach from the console window
        # and not print anything.
        set_property(TARGET "${name}" PROPERTY WIN32_EXECUTABLE FALSE)

        # QMLTest specifics
        qt_internal_extend_target("${name}" CONDITION arg_QMLTEST
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::QuickTest
        )

        qt_internal_extend_target("${name}" CONDITION arg_QMLTEST AND NOT ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )

        qt_internal_extend_target("${name}" CONDITION arg_QMLTEST AND ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR=":/"
        )

        # Android requires Qt::Gui so add it by default for tests
        qt_internal_extend_target("${name}" CONDITION ANDROID
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Gui
        )
    endif()

    foreach(path IN LISTS arg_QML_IMPORTPATH)
        list(APPEND extra_test_args "-import" "${path}")
    endforeach()

    # Generate a label in the form tests/auto/foo/bar/tst_baz
    # and use it also for XML output
    file(RELATIVE_PATH label "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    if (arg_LOWDPI)
        target_compile_definitions("${name}" PUBLIC TESTCASE_LOWDPI)
        if (MACOS)
            set_property(TARGET "${name}" PROPERTY MACOSX_BUNDLE_INFO_PLIST "${QT_MKSPECS_DIR}/macx-clang/Info.plist.disable_highdpi")
            set_property(TARGET "${name}" PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
        endif()
    endif()

    if (ANDROID)
        qt_internal_android_test_arguments("${name}" test_executable extra_test_args)
        set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
    else()
        if(arg_QMLTEST AND NOT arg_SOURCES)
            set(test_working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
            set(test_executable ${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner)
        else()
            if (arg_WORKING_DIRECTORY)
                set(test_working_dir "${arg_WORKING_DIRECTORY}")
            elseif(arg_OUTPUT_DIRECTORY)
                set(test_working_dir "${arg_OUTPUT_DIRECTORY}")
            else()
                set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
            endif()
            set(test_executable "${name}")
        endif()

        if (NOT arg_CATCH)
            #TODO: Should we replace this by TESTARGS environment variable?
            list(APPEND extra_test_args "-o" "${name}.xml,xml" "-o" "-,txt")
        endif()
    endif()

    qt_internal_collect_command_environment(test_env_path test_env_plugin_path)

    if(arg_NO_WRAPPER OR QT_NO_TEST_WRAPPERS)
        add_test(NAME "${name}" COMMAND ${test_executable} ${extra_test_args}
                WORKING_DIRECTORY "${test_working_dir}")
        set_property(TEST "${name}" APPEND PROPERTY
                     ENVIRONMENT "PATH=${test_env_path}"
                                 "QT_TEST_RUNNING_IN_CTEST=1"
                                 "QT_PLUGIN_PATH=${test_env_plugin_path}"
        )
    else()
        set(test_wrapper_file "${CMAKE_CURRENT_BINARY_DIR}/${name}Wrapper$<CONFIG>.cmake")
        qt_internal_create_test_script(NAME "${name}"
                               COMMAND "${test_executable}"
                               ARGS "${extra_test_args}"
                               WORKING_DIRECTORY "${test_working_dir}"
                               OUTPUT_FILE "${test_wrapper_file}"
                               ENVIRONMENT "QT_TEST_RUNNING_IN_CTEST" 1
                                           "PATH" "${test_env_path}"
                                           "QT_PLUGIN_PATH" "${test_env_plugin_path}"
        )
    endif()

    if(arg_QT_TEST_SERVER_LIST AND NOT ANDROID)
        qt_internal_setup_docker_test_fixture(${name} ${arg_QT_TEST_SERVER_LIST})
    endif()

    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}" LABELS "${label}")
    if (arg_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${arg_TIMEOUT})
    endif()

    # Add a ${target}/check makefile target, to more easily test one test.
    add_custom_target("${name}_check"
        VERBATIM
        COMMENT "Running ${CMAKE_CTEST_COMMAND} -V -R \"^${name}$\""
        COMMAND "${CMAKE_CTEST_COMMAND}" -V -R "^${name}$"
    )
    if(TARGET "${name}")
        add_dependencies("${name}_check" "${name}")
    endif()

    if(ANDROID OR IOS OR WINRT)
        set(builtin_testdata TRUE)
    endif()

    if(builtin_testdata)
        # Safe guard against qml only tests, no source files == no target
        if (TARGET "${name}")
            target_compile_definitions("${name}" PRIVATE BUILTIN_TESTDATA)

            foreach(testdata IN LISTS arg_TESTDATA)
                list(APPEND builtin_files ${testdata})
            endforeach()

            set(blacklist_path "BLACKLIST")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${blacklist_path}")
                list(APPEND builtin_files ${blacklist_path})
            endif()

            list(REMOVE_DUPLICATES builtin_files)

            # Skip Qt quick compiler when embedding test resources
            foreach(file IN LISTS builtin_files)
                set_source_files_properties(${file}
                    PROPERTIES QT_SKIP_QUICKCOMPILER TRUE
                )
            endforeach()

            if (builtin_files)
                qt_internal_add_resource(${name} "${name}_testdata_builtin"
                    PREFIX "/"
                    FILES ${builtin_files}
                    BASE ${CMAKE_CURRENT_SOURCE_DIR})
            endif()
        endif()
    else()
        # Install test data
        file(RELATIVE_PATH relative_path_to_test_project
            "${QT_TOP_LEVEL_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        qt_path_join(testdata_install_dir ${QT_INSTALL_DIR}
                     "${relative_path_to_test_project}")
        foreach(testdata IN LISTS arg_TESTDATA)
            set(testdata "${CMAKE_CURRENT_SOURCE_DIR}/${testdata}")
            if (IS_DIRECTORY "${testdata}")
                qt_install(
                    DIRECTORY "${testdata}"
                    DESTINATION "${testdata_install_dir}")
            else()
                qt_install(
                    FILES "${testdata}"
                    DESTINATION "${testdata_install_dir}")
            endif()
        endforeach()
    endif()

endfunction()

# This function adds test with specified NAME and wraps given test COMMAND with standalone cmake
# script.
#
# NAME must be compatible with add_test function, since it's propagated as is.
# COMMAND might be either target or path to executable. When test is called either by ctest or
# directly by 'cmake -P path/to/scriptWrapper.cmake', COMMAND will be executed in specified
# WORKING_DIRECTORY with arguments specified in ARGS.
#
# See also qt_internal_create_command_script for details.
function(qt_internal_create_test_script)
    #This style of parsing keeps ';' in ENVIRONMENT variables
    cmake_parse_arguments(PARSE_ARGV 0 arg
                          ""
                          "NAME;COMMAND;OUTPUT_FILE;WORKING_DIRECTORY"
                          "ARGS;ENVIRONMENT;PRE_RUN;POST_RUN"
    )

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "qt_internal_create_test_script: Test COMMAND is not specified")
    endif()

    if(NOT arg_NAME)
        message(FATAL_ERROR "qt_internal_create_test_script: Test NAME is not specified")
    endif()

    if(NOT arg_OUTPUT_FILE)
        message(FATAL_ERROR "qt_internal_create_test_script: Test Wrapper OUTPUT_FILE\
is not specified")
    endif()

    if(arg_PRE_RUN)
        message(WARNING "qt_internal_create_test_script: PRE_RUN is not acceptable argument\
for this function. Will be ignored")
    endif()

    if(arg_POST_RUN)
        message(WARNING "qt_internal_create_test_script: POST_RUN is not acceptable argument\
for this function. Will be ignored")
    endif()

    if(arg_ARGS)
        set(command_args ${arg_ARGS})# Avoid "${arg_ARGS}" usage and let cmake expand string to
                                    # semicolon-separated list
        qt_internal_wrap_command_arguments(command_args)
    endif()

    if(TARGET ${arg_COMMAND})
        set(executable_file "$<TARGET_FILE:${arg_COMMAND}>")
    else()
        set(executable_file "${arg_COMMAND}")
    endif()

    add_test(NAME "${arg_NAME}" COMMAND "${CMAKE_COMMAND}" "-P" "${arg_OUTPUT_FILE}"
                WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}")

    # If crosscompiling is enabled, we should avoid run cmake in emulator environment.
    # Prepend emulator to test command in generated cmake script instead. Keep in mind that
    # CROSSCOMPILING_EMULATOR don't check if actual cross compilation is configured,
    # emulator is prepended independently.
    if(CMAKE_CROSSCOMPILING)
        get_test_property(${arg_NAME} CROSSCOMPILING_EMULATOR crosscompiling_emulator)
        if(NOT crosscompiling_emulator)
            set(crosscompiling_emulator "")
        else()
            qt_internal_wrap_command_arguments(crosscompiling_emulator)
        endif()
    endif()

    qt_internal_create_command_script(COMMAND "${crosscompiling_emulator} \${env_test_runner} \
\"${executable_file}\" \${env_test_args} ${command_args}"
                                      OUTPUT_FILE "${arg_OUTPUT_FILE}"
                                      WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}"
                                      ENVIRONMENT ${arg_ENVIRONMENT}
                                      PRE_RUN "separate_arguments(env_test_args NATIVE_COMMAND \
\"\$ENV{TESTARGS}\")"
                                              "separate_arguments(env_test_runner NATIVE_COMMAND \
\"\$ENV{TESTRUNNER}\")"
    )
endfunction()

# This function wraps COMMAND with cmake script, that makes possible standalone run with external
# arguments.
#
# Generated wrapper will be written to OUTPUT_FILE.
# If WORKING_DIRECTORY is not set COMMAND will be executed in CMAKE_CURRENT_BINARY_DIR.
# Variables from ENVIRONMENT will be set before COMMAND execution.
# PRE_RUN and POST_RUN arguments may contain extra cmake code that supposed to be executed before
# and after COMMAND, respectively. Both arguments accept a list of cmake script language
# constructions. Each item of the list will be concantinated into single string with '\n' sepatator.
function(qt_internal_create_command_script)
    #This style of parsing keeps ';' in ENVIRONMENT variables
    cmake_parse_arguments(PARSE_ARGV 0 arg
                          ""
                          "OUTPUT_FILE;WORKING_DIRECTORY"
                          "COMMAND;ENVIRONMENT;PRE_RUN;POST_RUN"
    )

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "qt_internal_create_command_script: COMMAND is not specified")
    endif()

    if(NOT arg_OUTPUT_FILE)
        message(FATAL_ERROR "qt_internal_create_command_script: Wrapper OUTPUT_FILE\
is not specified")
    endif()

    if(NOT arg_WORKING_DIRECTORY)
        set(arg_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(environment_extras)
    set(skipNext false)
    if(arg_ENVIRONMENT)
        list(LENGTH arg_ENVIRONMENT length)
        math(EXPR length "${length} - 1")
        foreach(envIdx RANGE ${length})
            if(skipNext)
                set(skipNext FALSE)
                continue()
            endif()

            set(envVariable "")
            set(envValue "")

            list(GET arg_ENVIRONMENT ${envIdx} envVariable)
            math(EXPR envIdx "${envIdx} + 1")
            if (envIdx LESS_EQUAL ${length})
                list(GET arg_ENVIRONMENT ${envIdx} envValue)
            endif()

            if(NOT "${envVariable}" STREQUAL "")
                set(environment_extras "${environment_extras}\nset(ENV{${envVariable}} \
\"${envValue}\")")
            endif()
            set(skipNext TRUE)
        endforeach()
    endif()

    #Escaping environment variables before expand them by file GENERATE
    string(REPLACE "\\" "\\\\" environment_extras "${environment_extras}")

    if(WIN32)
        # It's necessary to call actual test inside 'cmd.exe', because 'execute_process' uses
        # SW_HIDE to avoid showing a console window, it affects other GUI as well.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/17690 for details.
        set(extra_runner "cmd /c")
    endif()

    if(arg_PRE_RUN)
        string(JOIN "\n" pre_run ${arg_PRE_RUN})
    endif()

    if(arg_POST_RUN)
        string(JOIN "\n" post_run ${arg_POST_RUN})
    endif()

    file(GENERATE OUTPUT "${arg_OUTPUT_FILE}" CONTENT
"#!${CMAKE_COMMAND} -P
# Qt generated command wrapper

${environment_extras}
${pre_run}
execute_process(COMMAND ${extra_runner} ${arg_COMMAND}
                WORKING_DIRECTORY \"${arg_WORKING_DIRECTORY}\"
                RESULT_VARIABLE result
)
${post_run}
if(NOT result EQUAL 0)
    string(JOIN \" \" full_command ${arg_COMMAND})
    message(FATAL_ERROR \"\${full_command} execution failed.\")
endif()"
    )
endfunction()

# This function creates an executable for use as a helper program with tests. Some
# tests launch separate programs to test certain input/output behavior.
# Specify OVERRIDE_OUTPUT_DIRECTORY if you dont' want to place the helper in the parent directory,
# in which case you should specify OUTPUT_DIRECTORY "/foo/bar" manually.
function(qt_internal_add_test_helper name)

    set(qt_add_test_helper_optional_args
        "OVERRIDE_OUTPUT_DIRECTORY"
    )

    qt_parse_all_arguments(arg "qt_add_test_helper"
        "${qt_add_test_helper_optional_args};${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
         ${ARGN})

    qt_remove_args(forward_args
        ARGS_TO_REMOVE
            "${name}"
            ${qt_add_test_helper_optional_args}
        ALL_ARGS
            ${qt_add_test_helper_optional_args}
            ${__qt_internal_add_executable_optional_args}
            ${__qt_internal_add_executable_single_args}
            ${__qt_internal_add_executable_multi_args}
        ARGS
            ${ARGV}
    )

    set(extra_args_to_pass)
    if(NOT arg_OVERRIDE_OUTPUT_DIRECTORY)
        set(extra_args_to_pass OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
    endif()

    qt_internal_add_executable("${name}" NO_INSTALL ${extra_args_to_pass} ${forward_args})
endfunction()

function(qt_internal_wrap_command_arguments argument_list)
    list(TRANSFORM ${argument_list} REPLACE "^(.+)$" "[=[\\1]=]")
    list(JOIN ${argument_list} " " ${argument_list})
    set(${argument_list} "${${argument_list}}" PARENT_SCOPE)
endfunction()

function(qt_internal_collect_command_environment out_path out_plugin_path)
    # Get path to <qt_relocatable_install_prefix>/bin, as well as CMAKE_INSTALL_PREFIX/bin, and
    # combine them with the PATH environment variable.
    # It's needed on Windows to find the shared libraries and plugins.
    # qt_relocatable_install_prefix is dynamically computed from the location of where the Qt CMake
    # package is found.
    # The regular CMAKE_INSTALL_PREFIX can be different for example when building standalone tests.
    # Any given CMAKE_INSTALL_PREFIX takes priority over qt_relocatable_install_prefix for the
    # PATH environment variable.
    set(install_prefixes "${CMAKE_INSTALL_PREFIX}")
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        list(APPEND install_prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    endif()

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" test_env_path)
    foreach(install_prefix ${install_prefixes})
        file(TO_NATIVE_PATH "${install_prefix}/${INSTALL_BINDIR}" install_prefix)
        set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}${install_prefix}")
    endforeach()
    set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}$ENV{PATH}")
    string(REPLACE ";" "\;" test_env_path "${test_env_path}")
    set(${out_path} "${test_env_path}" PARENT_SCOPE)

    # Add the install prefix to list of plugin paths when doing a prefix build
    if(NOT QT_INSTALL_DIR)
        foreach(install_prefix ${install_prefixes})
            file(TO_NATIVE_PATH "${install_prefix}/${INSTALL_BINDIR}" install_prefix)
            list(APPEND plugin_paths "${install_prefix}")
        endforeach()
    endif()

    #TODO: Collect all paths from known repositories when performing a super
    # build.
    file(TO_NATIVE_PATH "${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}" install_pluginsdir)
    list(APPEND plugin_paths "${install_pluginsdir}")
    list(JOIN plugin_paths "${QT_PATH_SEPARATOR}" plugin_paths_joined)
    string(REPLACE ";" "\;" plugin_paths_joined "${plugin_paths_joined}")
    set(${out_plugin_path} "${plugin_paths_joined}" PARENT_SCOPE)
endfunction()
