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

# Collects CMAKE_MODULE_PATH from QT_ADDITIONAL_PACKAGES_PREFIX_PATH
function(__qt_internal_collect_additional_module_paths)
    if(__qt_additional_module_paths_set)
        return()
    endif()
    foreach(prefix_path IN LISTS QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        list(APPEND CMAKE_MODULE_PATH "${prefix_path}/${QT_CMAKE_EXPORT_NAMESPACE}")
        # TODO: Need to consider the INSTALL_LIBDIR value when collecting CMAKE_MODULE_PATH.
        # See QTBUG-123039.
        list(APPEND CMAKE_MODULE_PATH "${prefix_path}/lib/cmake/${QT_CMAKE_EXPORT_NAMESPACE}")
    endforeach()
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
    set(__qt_additional_module_paths_set TRUE PARENT_SCOPE)
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
