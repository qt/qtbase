function(_qt_internal_generate_tool_command_wrapper)
    get_property(is_called GLOBAL PROPERTY _qt_internal_generate_tool_command_wrapper_called)
    if(NOT CMAKE_HOST_WIN32 OR is_called)
        return()
    endif()

    set(prefixes "")

    # In a prefix build, the just-built tools should pick up libraries from the current repo build
    # dir.
    if(QT_BUILD_DIR)
        list(APPEND prefixes "${QT_BUILD_DIR}")
    endif()

    # Pick up libraries from the main location where Qt was installed during a Qt build.
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        list(APPEND prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    endif()

    # Needed for ExternalProjects examples, where the Qt build dir is passed via this variable
    # to the example project.
    if(QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        __qt_internal_prefix_paths_to_roots(additional_roots
            "${QT_ADDITIONAL_PACKAGES_PREFIX_PATH}")
        list(APPEND prefixes ${QT_ADDITIONAL_PACKAGES_PREFIX_PATH})
    endif()

    # Fallback to wherever Qt6 package is.
    if(QT6_INSTALL_PREFIX)
        list(APPEND prefixes "${QT6_INSTALL_PREFIX}")
    endif()

    # When building qtbase, QT6_INSTALL_BINS is not set yet.
    if(INSTALL_BINDIR)
        set(bin_suffix "${INSTALL_BINDIR}")
    else()
        set(bin_suffix "${QT6_INSTALL_BINS}")
    endif()

    set(path_dirs "")
    foreach(prefix IN LISTS prefixes)
        set(bin_dir "${prefix}/${bin_suffix}")
        if(EXISTS "${bin_dir}")
            file(TO_NATIVE_PATH "${bin_dir}" path_dir)
            list(APPEND path_dirs "${path_dir}")
        endif()
    endforeach()

    set(tool_command_wrapper_dir "${CMAKE_BINARY_DIR}/.qt/bin")
    file(MAKE_DIRECTORY "${tool_command_wrapper_dir}")
    set(tool_command_wrapper_path "${tool_command_wrapper_dir}/qt_setup_tool_path.bat")

    file(WRITE "${tool_command_wrapper_path}" "@echo off
set PATH=${path_dirs};%PATH%
%*")

    set(QT_TOOL_COMMAND_WRAPPER_PATH "${tool_command_wrapper_path}"
        CACHE INTERNAL "Path to the wrapper of the tool commands")

    set_property(GLOBAL PROPERTY _qt_internal_generate_tool_command_wrapper_called TRUE)
endfunction()

# Wraps a tool command with a script that contains the necessary environment for the tool to run
# correctly.
# _qt_internal_wrap_tool_command(var <SET|APPEND> <command> [args...])
# Arguments:
#    APPEND Selects the 'append' mode for the out_variable argument.
#    SET Selects the 'set' mode for the out_variable argument.
#
# FIXME: Replace all usages of _qt_internal_wrap_tool_command
# with _qt_internal_get_wrap_tool_script_path and remove the former.
# The former always adds the COMMAND keyword, which does not allow the caller to wrap the
# commands in a generator expression. See _qt_internal_target_enable_qmllint for an example.
function(_qt_internal_wrap_tool_command out_variable action)
    set(append FALSE)
    if(action STREQUAL "APPEND")
        set(append TRUE)
    elseif(NOT action STREQUAL "SET")
        message(FATAL_ERROR "Invalid action specified ${action}. Supported actions: SET, APPEND")
    endif()

    # Ensure the script wrapper exists.
    _qt_internal_generate_tool_command_wrapper()

    set(cmd COMMAND ${QT_TOOL_COMMAND_WRAPPER_PATH} ${ARGN})

    if(append)
        list(APPEND ${out_variable} ${cmd})
    else()
        set(${out_variable} ${cmd})
    endif()
    set(${out_variable} "${${out_variable}}" PARENT_SCOPE)
endfunction()

# Gets the path to tool wrapper shell script.
function(_qt_internal_get_tool_wrapper_script_path out_variable)
    # Ensure the script wrapper exists.
    _qt_internal_generate_tool_command_wrapper()

    set(${out_variable} "${QT_TOOL_COMMAND_WRAPPER_PATH}" PARENT_SCOPE)
endfunction()
