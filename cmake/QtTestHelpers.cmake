# Simple wrapper around qt_add_executable for benchmarks which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_add_executable() for more details.
function(qt_add_benchmark target)

    qt_parse_all_arguments(arg "qt_add_benchmark"
        "${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_add_executable_optional_args}"
            "${__qt_add_executable_single_args}"
            "${__qt_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

endfunction()

# Simple wrapper around qt_add_executable for manual tests which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_add_executable() for more details.
function(qt_add_manual_test target)

    qt_parse_all_arguments(arg "qt_add_manual_test"
        "${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_add_executable_optional_args}"
            "${__qt_add_executable_single_args}"
            "${__qt_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

endfunction()


# This function creates a CMake test target with the specified name for use with CTest.
function(qt_add_test name)
    qt_parse_all_arguments(arg "qt_add_test"
        "RUN_SERIAL;EXCEPTIONS;GUI;QMLTEST;CATCH;LOWDPI"
        "OUTPUT_DIRECTORY;WORKING_DIRECTORY;TIMEOUT;VERSION"
        "QML_IMPORTPATH;TESTDATA;${__default_private_args};${__default_public_args}" ${ARGN}
    )

    if (NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if (${arg_EXCEPTIONS})
        set(exceptions_text "EXCEPTIONS")
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

        qt_add_executable("${name}"
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

        qt_extend_target("${name}" CONDITION arg_QMLTEST
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::QuickTest
        )

        qt_extend_target("${name}" CONDITION arg_QMLTEST AND NOT ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )

        qt_extend_target("${name}" CONDITION arg_QMLTEST AND ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR=":/"
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
        qt_android_add_test("${name}")
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
            list(APPEND test_outputs "-o" "${name}.xml,xml" "-o" "-,txt")
        endif()

        add_test(NAME "${name}" COMMAND ${test_executable} ${extra_test_args} ${test_outputs} WORKING_DIRECTORY "${test_working_dir}")
    endif()
    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}" LABELS "${label}")
    if (arg_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${arg_TIMEOUT})
    endif()

    # Add a ${target}/check makefile target, to more easily test one test.
    if(TEST "${name}")
        add_custom_target("${name}_check"
            VERBATIM
            COMMENT "Running ${CMAKE_CTEST_COMMAND} -V -R \"^${name}$\""
            COMMAND "${CMAKE_CTEST_COMMAND}" -V -R "^${name}$"
            )
        if(TARGET "${name}")
            add_dependencies("${name}_check" "${name}")
        endif()
    endif()

    # Get path to <qt_relocatable_install_prefix>/bin, as well as CMAKE_INSTALL_PREFIX/bin, then
    # prepend them to the PATH environment variable.
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

    set(test_env_path "PATH=${CMAKE_CURRENT_BINARY_DIR}")
    foreach(install_prefix ${install_prefixes})
        set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}${install_prefix}/${INSTALL_BINDIR}")
    endforeach()
    set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}$ENV{PATH}")
    string(REPLACE ";" "\;" test_env_path "${test_env_path}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "${test_env_path}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_TEST_RUNNING_IN_CTEST=1")

    # Add the install prefix to list of plugin paths when doing a prefix build
    if(NOT QT_INSTALL_DIR)
        foreach(install_prefix ${install_prefixes})
            list(APPEND plugin_paths "${install_prefix}/${INSTALL_PLUGINSDIR}")
        endforeach()
    endif()

    #TODO: Collect all paths from known repositories when performing a super
    # build.
    list(APPEND plugin_paths "${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}")
    list(JOIN plugin_paths "${QT_PATH_SEPARATOR}" plugin_paths_joined)
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_PLUGIN_PATH=${plugin_paths_joined}")

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
                qt_add_resource(${name} "${name}_testdata_builtin"
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


# This function creates an executable for use as a helper program with tests. Some
# tests launch separate programs to test certain input/output behavior.
# Specify OVERRIDE_OUTPUT_DIRECTORY if you dont' want to place the helper in the parent directory,
# in which case you should specify OUTPUT_DIRECTORY "/foo/bar" manually.
function(qt_add_test_helper name)

    set(qt_add_test_helper_optional_args
        "OVERRIDE_OUTPUT_DIRECTORY"
    )

    qt_parse_all_arguments(arg "qt_add_test_helper"
        "${qt_add_test_helper_optional_args};${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
         ${ARGN})

    qt_remove_args(forward_args
        ARGS_TO_REMOVE
            "${name}"
            ${qt_add_test_helper_optional_args}
        ALL_ARGS
            ${qt_add_test_helper_optional_args}
            ${__qt_add_executable_optional_args}
            ${__qt_add_executable_single_args}
            ${__qt_add_executable_multi_args}
        ARGS
            ${ARGV}
    )

    set(extra_args_to_pass)
    if(NOT arg_OVERRIDE_OUTPUT_DIRECTORY)
        set(extra_args_to_pass OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
    endif()

    qt_add_executable("${name}" NO_INSTALL ${extra_args_to_pass} ${forward_args})
endfunction()
