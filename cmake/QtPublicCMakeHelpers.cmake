# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# copy_if_different works incorrect in Windows if file size if bigger than 2GB.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/23052 and QTBUG-99491 for details.
function(_qt_internal_copy_file_if_different_command out_var src_file dst_file)
    # The CMake version higher than 3.23 doesn't contain the issue
    if(CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_LESS 3.23)
        set(${out_var} "${CMAKE_COMMAND}"
            "-DSRC_FILE_PATH=${src_file}"
            "-DDST_FILE_PATH=${dst_file}"
            -P "${_qt_6_config_cmake_dir}/QtCopyFileIfDifferent.cmake"
            PARENT_SCOPE
        )
    else()
        set(${out_var} "${CMAKE_COMMAND}"
            -E copy_if_different
            "${src_file}"
            "${dst_file}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# The function checks if add_custom_command has the support of the DEPFILE argument.
function(_qt_internal_check_depfile_support out_var)
    if(CMAKE_GENERATOR MATCHES "Ninja" OR
        (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 AND CMAKE_GENERATOR MATCHES "Makefiles")
        OR (CMAKE_VERSION VERSION_GREATER_EQUAL 3.21
        AND (CMAKE_GENERATOR MATCHES "Xcode"
            OR (CMAKE_GENERATOR MATCHES "Visual Studio ([0-9]+)" AND CMAKE_MATCH_1 GREATER_EQUAL 12)
            )
        )
    )
        set(${out_var} TRUE)
    else()
        set(${out_var} FALSE)
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Collect additional package prefix paths to look for Qt packages, both from command line and the
# env variable ${prefixes_var}. The result is stored in ${out_var} and is a list of paths ending
# with "/lib/cmake".
function(__qt_internal_collect_additional_prefix_paths out_var prefixes_var)
    if(DEFINED "${out_var}")
        return()
    endif()

    set(additional_packages_prefix_paths "")

    set(additional_packages_prefixes "")
    if(${prefixes_var})
        list(APPEND additional_packages_prefixes ${${prefixes_var}})
    endif()
    if(DEFINED ENV{${prefixes_var}}
        AND NOT "$ENV{${prefixes_var}}" STREQUAL "")
        set(prefixes_from_env "$ENV{${prefixes_var}}")
        if(NOT CMAKE_HOST_WIN32)
            string(REPLACE ":" ";" prefixes_from_env "${prefixes_from_env}")
        endif()
        list(APPEND additional_packages_prefixes ${prefixes_from_env})
    endif()

    foreach(additional_path IN LISTS additional_packages_prefixes)
        file(TO_CMAKE_PATH "${additional_path}" additional_path)

        # The prefix paths need to end with lib/cmake to ensure the packages are found when
        # cross compiling. Search for REROOT_PATH_ISSUE_MARKER in the qt.toolchain.cmake file for
        # details.
        # We must pass the values via the PATHS options because the main find_package call uses
        # NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH values are discarded.
        # CMAKE_FIND_ROOT_PATH values are not discarded and togegher with the PATHS option, it
        # ensures packages from additional prefixes are found.
        if(NOT additional_path MATCHES "/lib/cmake$")
            string(APPEND additional_path "/lib/cmake")
        endif()
        list(APPEND additional_packages_prefix_paths "${additional_path}")
    endforeach()

    set("${out_var}" "${additional_packages_prefix_paths}" PARENT_SCOPE)
endfunction()

# Take a list of prefix paths ending with "/lib/cmake", and return a list of absolute paths with
# "/lib/cmake" removed.
function(__qt_internal_prefix_paths_to_roots out_var prefix_paths)
    set(result "")
    foreach(path IN LISTS prefix_paths)
        if(path MATCHES "/lib/cmake$")
            string(APPEND path "/../..")
        endif()
        get_filename_component(path "${path}" ABSOLUTE)
        list(APPEND result "${path}")
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

# This function gets all targets below this directory
#
# Multi-value Arguments:
#     EXCLUDE list of target types that should be filtered from resulting list.
#
#     INCLUDE list of target types that should be filtered from resulting list.
#             EXCLUDE has higher priority than INCLUDE.
function(_qt_internal_collect_buildsystem_targets result dir)
    cmake_parse_arguments(arg "" "" "EXCLUDE;INCLUDE" ${ARGN})

    if(NOT _qt_internal_collect_buildsystem_targets_inner)
        set(${result} "")
        set(_qt_internal_collect_buildsystem_targets_inner TRUE)
    endif()

    set(forward_args "")
    if(arg_EXCLUDE)
        set(forward_args APPEND EXCLUDE ${arg_EXCLUDE})
    endif()

    if(arg_INCLUDE)
        set(forward_args APPEND INCLUDE ${arg_INCLUDE})
    endif()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

    # Make sure that we don't hit endless recursion when running qt-cmake-standalone-test on a
    # in-source test dir, where the currently processed directory lists itself in its SUBDIRECTORIES
    # property.
    # See https://bugreports.qt.io/browse/QTBUG-119998
    # and https://gitlab.kitware.com/cmake/cmake/-/issues/25489
    # Do it only when QT_INTERNAL_IS_STANDALONE_TEST is set, to avoid the possible slowdown when
    # processing many subdirectores when configuring all standalone tests rather than just one.
    if(QT_INTERNAL_IS_STANDALONE_TEST)
        list(REMOVE_ITEM subdirs "${dir}")
    endif()

    foreach(subdir IN LISTS subdirs)
        _qt_internal_collect_buildsystem_targets(${result} "${subdir}" ${forward_args})
    endforeach()
    get_property(sub_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(real_targets "")
    if(sub_targets)
        foreach(target IN LISTS sub_targets)
            get_target_property(target_type ${target} TYPE)
            if((NOT arg_INCLUDE OR target_type IN_LIST arg_INCLUDE) AND
                (NOT arg_EXCLUDE OR (NOT target_type IN_LIST arg_EXCLUDE)))
                list(APPEND real_targets ${target})
            endif()
        endforeach()
    endif()
    set(${result} ${${result}} ${real_targets} PARENT_SCOPE)
endfunction()

# Add a custom target ${target} that is *not* added to the default build target in a safe way.
# Dependencies must then be added with _qt_internal_add_phony_target_dependencies.
#
# What's "safe" in this context? For the Visual Studio generators, we cannot use add_dependencies,
# because this would enable the dependencies in the default build of the solution. See QTBUG-115166
# and upstream CMake issue #16668 for details. Instead, we record the dependencies (added with
# _qt_internal_add_phony_target_dependencies) and create the target at the end of the top-level
# directory scope.
#
# This only works if at least CMake 3.19 is used. Older CMake versions will trigger a warning that
# can be turned off with the variable ${WARNING_VARIABLE}.
#
# For other generators, this is just a call to add_custom_target, unless the target already exists,
# followed by add_dependencies.
#
# Use this function for targets that are not part of the default build, i.e. that should be
# triggered by the user.
#
# TARGET_CREATED_HOOK is the name of a function that is called after the target has been created.
# It takes the target's name as first and only argument.
#
# Example:
#     _qt_internal_add_phony_target(update_translations
#          WARNING_VARIABLE QT_NO_GLOBAL_LUPDATE_TARGET_CREATED_WARNING
#     )
#     _qt_internal_add_phony_target_dependencies(update_translations
#          narf_lupdate_zort_lupdate
#     )
#
function(_qt_internal_add_phony_target target)
    set(no_value_options "")
    set(single_value_options
        TARGET_CREATED_HOOK
        WARNING_VARIABLE
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if("${arg_WARNING_VARIABLE}" STREQUAL "")
        message(FATAL_ERROR "WARNING_VARIABLE must be provided.")
    endif()
    if(CMAKE_GENERATOR MATCHES "^Visual Studio ")
        if(${CMAKE_VERSION} VERSION_LESS "3.19.0")
            if(NOT ${${arg_WARNING_VARIABLE}})
                message(WARNING
                    "Cannot create target ${target} with this CMake version. "
                    "Please upgrade to CMake 3.19.0 or newer. "
                    "Set ${WARNING_VARIABLE} to ON to disable this warning."
                )
            endif()
            return()
        endif()

        get_property(already_deferred GLOBAL PROPERTY _qt_target_${target}_creation_deferred)
        if(NOT already_deferred)
            cmake_language(EVAL CODE
                "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\" CALL _qt_internal_add_phony_target_deferred \"${target}\")"
            )
            if(DEFINED arg_TARGET_CREATED_HOOK)
                set_property(GLOBAL
                    PROPERTY _qt_target_${target}_creation_hook ${arg_TARGET_CREATED_HOOK}
                )
            endif()
        endif()
        set_property(GLOBAL APPEND PROPERTY _qt_target_${target}_creation_deferred ON)
    else()
        if(NOT TARGET ${target})
            add_custom_target(${target})
            if(DEFINED arg_TARGET_CREATED_HOOK)
                if(CMAKE_VERSION VERSION_LESS "3.19")
                    set(incfile
                        "${CMAKE_CURRENT_BINARY_DIR}/.qt_internal_add_phony_target.cmake"
                    )
                    file(WRITE "${incfile}" "${arg_TARGET_CREATED_HOOK}(${target})")
                    include("${incfile}")
                    file(REMOVE "${incfile}")
                else()
                    cmake_language(CALL "${arg_TARGET_CREATED_HOOK}" "${target}")
                endif()
            endif()
        endif()
    endif()
endfunction()

# Adds dependencies to a custom target that has been created with
# _qt_internal_add_phony_target. See the docstring at _qt_internal_add_phony_target for
# more details.
function(_qt_internal_add_phony_target_dependencies target)
    set(dependencies ${ARGN})
    if(CMAKE_GENERATOR MATCHES "^Visual Studio ")
        set_property(GLOBAL APPEND PROPERTY _qt_target_${target}_dependencies ${dependencies})

        # Exclude the dependencies from the solution's default build to avoid them being enabled
        # accidentally should the user add another dependency to them.
        set_target_properties(${dependencies} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
    else()
        add_dependencies(${target} ${dependencies})
    endif()
endfunction()

# Hack for the Visual Studio generator. Create the custom target named ${target} and work
# around the lack of a working add_dependencies by calling 'cmake --build' for every dependency.
function(_qt_internal_add_phony_target_deferred target)
    get_property(target_dependencies GLOBAL PROPERTY _qt_target_${target}_dependencies)
    set(target_commands "")
    foreach(dependency IN LISTS target_dependencies)
        list(APPEND target_commands
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t ${dependency}
        )
    endforeach()
    add_custom_target(${target} ${target_commands})
    get_property(creation_hook GLOBAL PROPERTY _qt_target_${target}_creation_hook)
    if(creation_hook)
        cmake_language(CALL ${creation_hook} ${target})
    endif()
endfunction()
