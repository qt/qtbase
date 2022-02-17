# NOTE: This code should only ever be executed in script mode. It expects to be
#       used either as part of an install(CODE) call or called by a script
#       invoked via cmake -P as a POST_BUILD step.

cmake_minimum_required(VERSION 3.16...3.21)

# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_deploy_qt_conf qt_conf_absolute_path)
    set(no_value_options "")
    set(single_value_options
        PREFIX
        DOC_DIR
        HEADERS_DIR
        LIB_DIR
        LIBEXEC_DIR
        BIN_DIR
        PLUGINS_DIR
        QML_DIR
        ARCHDATA_DIR
        DATA_DIR
        TRANSLATIONS_DIR
        EXAMPLES_DIR
        TESTS_DIR
        SETTINGS_DIR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT IS_ABSOLUTE "${qt_conf_absolute_path}")
        message(FATAL_ERROR
                "Given qt.conf path is not an absolute path: '${qt_conf_absolute_path}'")
    endif()

    # Only write out locations that differ from the defaults
    set(contents "[Paths]\n")
    if(arg_PREFIX)
        string(APPEND contents "Prefix = ${arg_PREFIX}\n")
    endif()
    if(arg_DOC_DIR AND NOT arg_DOC_DIR STREQUAL "doc")
        string(APPEND contents "Documentation = ${arg_DOC_DIR}\n")
    endif()
    if(arg_HEADERS_DIR AND NOT arg_HEADERS_DIR STREQUAL "include")
        string(APPEND contents "Headers = ${arg_HEADERS_DIR}\n")
    endif()
    if(arg_LIB_DIR AND NOT arg_LIB_DIR STREQUAL "lib")
        string(APPEND contents "Libraries = ${arg_LIB_DIR}\n")
    endif()

    # This one is special, the default is platform-specific
    if(arg_LIBEXEC_DIR AND
       ((WIN32 AND NOT arg_LIBEXEC_DIR STREQUAL "bin") OR
        (NOT WIN32 AND NOT arg_LIBEXEC_DIR STREQUAL "libexec")))
        string(APPEND contents "LibraryExecutables = ${arg_LIBEXEC_DIR}\n")
    endif()

    if(arg_BIN_DIR AND NOT arg_BIN_DIR STREQUAL "bin")
        string(APPEND contents "Binaries = ${arg_BIN_DIR}\n")
    endif()
    if(arg_PLUGINS_DIR AND NOT arg_PLUGINS_DIR STREQUAL "plugins")
        string(APPEND contents "Plugins = ${arg_PLUGINS_DIR}\n")
    endif()
    if(arg_QML_DIR AND NOT arg_QML_DIR STREQUAL "qml")
        string(APPEND contents "QmlImports = ${arg_QML_DIR}\n")
    endif()
    if(arg_ARCHDATA_DIR AND NOT arg_ARCHDATA_DIR STREQUAL ".")
        string(APPEND contents "ArchData = ${arg_ARCHDATA_DIR}\n")
    endif()
    if(arg_DATA_DIR AND NOT arg_DATA_DIR STREQUAL ".")
        string(APPEND contents "Data = ${arg_DATA_DIR}\n")
    endif()
    if(arg_TRANSLATIONS_DIR AND NOT arg_TRANSLATIONS_DIR STREQUAL "translations")
        string(APPEND contents "Translations = ${arg_TRANSLATIONS_DIR}\n")
    endif()
    if(arg_EXAMPLES_DIR AND NOT arg_EXAMPLES_DIR STREQUAL "examples")
        string(APPEND contents "Examples = ${arg_EXAMPLES_DIR}\n")
    endif()
    if(arg_TESTS_DIR AND NOT arg_TESTS_DIR STREQUAL "tests")
        string(APPEND contents "Tests = ${arg_TESTS_DIR}\n")
    endif()
    if(arg_SETTINGS_DIR AND NOT arg_SETTINGS_DIR STREQUAL ".")
        string(APPEND contents "Settings = ${arg_SETTINGS_DIR}\n")
    endif()

    message(STATUS "Writing ${qt_conf_absolute_path}")
    file(WRITE "${qt_conf_absolute_path}" "${contents}")
endfunction()

if(NOT __QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_deploy_qt_conf)
        if(__QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_deploy_qt_conf(${ARGV})
        else()
            message(FATAL_ERROR "qt_deploy_qt_conf() is only available in Qt 6.")
        endif()
    endfunction()
endif()

# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_deploy_runtime_dependencies)

    if(NOT __QT_DEPLOY_TOOL)
        message(FATAL_ERROR "No Qt deploy tool available for this target platform")
    endif()

    set(no_value_options
        GENERATE_QT_CONF
        VERBOSE
        NO_OVERWRITE
        NO_APP_STORE_COMPLIANCE   # TODO: Might want a better name
    )
    set(single_value_options
        EXECUTABLE
        BIN_DIR
        LIB_DIR
        PLUGINS_DIR
        QML_DIR
    )
    set(multi_value_options
        # These ADDITIONAL_... options are based on what file(GET_RUNTIME_DEPENDENCIES)
        # supports. We differentiate between the types of binaries so that we keep
        # open the possibility of switching to a purely CMake implementation of
        # the deploy tool based on file(GET_RUNTIME_DEPENDENCIES) instead of the
        # individual platform-specific tools (macdeployqt, windeployqt, etc.).
        ADDITIONAL_EXECUTABLES
        ADDITIONAL_LIBRARIES
        ADDITIONAL_MODULES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_EXECUTABLE)
        message(FATAL_ERROR "EXECUTABLE must be specified")
    endif()

    # None of these are used if the executable is a macOS app bundle
    if(NOT arg_BIN_DIR)
        set(arg_BIN_DIR "${QT_DEPLOY_BIN_DIR}")
    endif()
    if(NOT arg_LIB_DIR)
        set(arg_LIB_DIR "${QT_DEPLOY_LIB_DIR}")
    endif()
    if(NOT arg_QML_DIR)
        set(arg_QML_DIR "${QT_DEPLOY_QML_DIR}")
    endif()
    if(NOT arg_PLUGINS_DIR)
        set(arg_PLUGINS_DIR "${QT_DEPLOY_PLUGINS_DIR}")
    endif()

    # macdeployqt always writes out a qt.conf file. It will complain if one
    # already exists, so leave it to create it for us if we will be running it.
    if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
        # We might get EXECUTABLE pointing to either the actual binary under the
        # Contents/MacOS directory, or it might be pointing to the top of the
        # app bundle (i.e. the <appname>.app directory). We want the latter to
        # pass to macdeployqt.
        if(arg_EXECUTABLE MATCHES "^((.*/)?(.*).app)/Contents/MacOS/(.*)$")
            set(arg_EXECUTABLE "${CMAKE_MATCH_1}")
        endif()
    elseif(arg_GENERATE_QT_CONF)
        get_filename_component(exe_dir "${arg_EXECUTABLE}" DIRECTORY)
        if(exe_dir STREQUAL "")
            set(exe_dir ".")
            set(prefix  ".")
        else()
            string(REPLACE "/" ";" path "${exe_dir}")
            list(LENGTH path path_count)
            string(REPEAT "../" ${path_count} rel_path)
            string(REGEX REPLACE "/+$" "" prefix "${rel_path}")
        endif()
        qt6_deploy_qt_conf("${QT_DEPLOY_PREFIX}/${exe_dir}/qt.conf"
            PREFIX "${prefix}"
            BIN_DIR "${arg_BIN_DIR}"
            LIB_DIR "${arg_LIB_DIR}"
            PLUGINS_DIR "${arg_PLUGINS_DIR}"
            QML_DIR "${arg_QML_DIR}"
        )
    endif()

    set(extra_binaries_option "")
    set(tool_options "")

    if(arg_VERBOSE OR __QT_DEPLOY_VERBOSE)
        # macdeployqt supports 0-3: 0=no output, 1=error/warn (default), 2=normal, 3=debug
        # windeployqt supports 0-2: 0=error/warn (default), 1=verbose, 2=full_verbose
        if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Windows)
            list(APPEND tool_options --verbose 2)
        elseif(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
            list(APPEND tool_options -verbose=3)
        endif()
    endif()

    if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Windows)
        list(APPEND tool_options
            --dir       .
            --libdir    "${arg_BIN_DIR}"     # NOTE: Deliberately not arg_LIB_DIR
            --plugindir "${arg_PLUGINS_DIR}"
        )
        if(NOT arg_NO_OVERWRITE)
            list(APPEND tool_options --force)
        endif()
    elseif(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
        set(extra_binaries_option "-executable=")
        if(NOT arg_NO_APP_STORE_COMPLIANCE)
            list(APPEND tool_options -appstore-compliant)
        endif()
        if(NOT arg_NO_OVERWRITE)
            list(APPEND tool_options -always-overwrite)
        endif()
    endif()

    # This is an internal variable. It is normally unset and is only intended
    # for debugging purposes. It may be removed at any time without warning.
    list(APPEND tool_options ${__qt_deploy_tool_extra_options})

    # Both windeployqt and macdeployqt don't differentiate between the different
    # types of binaries, so we merge the lists and treat them all the same.
    # A purely CMake-based implementation would need to treat them differently
    # because of how file(GET_RUNTIME_DEPENDENCIES) works.
    set(additional_binaries
        ${arg_ADDITIONAL_EXECUTABLES}
        ${arg_ADDITIONAL_LIBRARIES}
        ${arg_ADDITIONAL_MODULES}
    )
    foreach(extra_binary IN LISTS additional_binaries)
        list(APPEND tool_options "${extra_binaries_option}${extra_binary}")
    endforeach()

    message(STATUS
        "Running Qt deploy tool for ${arg_EXECUTABLE} in working directory '${QT_DEPLOY_PREFIX}'")
    execute_process(
        COMMAND_ECHO STDOUT
        COMMAND "${__QT_DEPLOY_TOOL}" "${arg_EXECUTABLE}" ${tool_options}
        WORKING_DIRECTORY "${QT_DEPLOY_PREFIX}"
        RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Executing ${__QT_DEPLOY_TOOL} failed: ${result}")
    endif()

endfunction()

if(NOT __QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_deploy_runtime_dependencies)
        if(__QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_deploy_runtime_dependencies(${ARGV})
        else()
            message(FATAL_ERROR "qt_deploy_runtime_dependencies() is only available in Qt 6.")
        endif()
    endfunction()
endif()

function(_qt_internal_show_skip_runtime_deploy_message qt_build_type_string)
    message(STATUS
        "Skipping runtime deployment steps. "
        "Support for installing runtime dependencies is not implemented for "
        "this target platform (${__QT_DEPLOY_SYSTEM_NAME}, ${qt_build_type_string})."
    )
endfunction()
