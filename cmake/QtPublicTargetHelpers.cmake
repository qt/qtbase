# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(__qt_internal_strip_target_directory_scope_token target out_var)
    # In CMake versions earlier than CMake 3.18, a subdirectory scope id is appended to the
    # target name if the target is referenced in a target_link_libraries command from a
    # different directory scope than where the target was created.
    # Strip it.
    #
    # For informational purposes, in CMake 3.18, the target name looks as follows:
    # ::@(0x5604cb3f6b50);Threads::Threads;::@
    # This case doesn't have to be stripped (at least for now), because when we iterate over
    # link libraries, the tokens appear as separate target names.
    #
    # Example: Threads::Threads::@<0x5604cb3f6b50>
    # Output: Threads::Threads
    string(REGEX REPLACE "::@<.+>$" "" target "${target}")
    set("${out_var}" "${target}" PARENT_SCOPE)
endfunction()

# Tests if linker could resolve circular dependencies between object files and static libraries.
function(__qt_internal_static_link_order_public_test result)
    # We could trust iOS linker
    if(IOS)
        set(QT_HAVE_LINK_ORDER_MATTERS "FALSE" CACHE INTERNAL "Link order matters")
    endif()

    if(DEFINED QT_HAVE_LINK_ORDER_MATTERS)
        set(${result} "${QT_HAVE_LINK_ORDER_MATTERS}" PARENT_SCOPE)
        return()
    endif()

    if(EXISTS "${QT_CMAKE_DIR}")
        set(test_source_basedir "${QT_CMAKE_DIR}/..")
    else()
        set(test_source_basedir "${_qt_cmake_dir}/${QT_CMAKE_EXPORT_NAMESPACE}")
    endif()

    try_compile(${result}
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/static_link_order"
        "${test_source_basedir}/config.tests/static_link_order"
        static_link_order_test
        static_link_order_test
    )
    message(STATUS "Check if linker can resolve circular dependencies - ${${result}}")

    # Invert the result
    if(${result})
        set(${result} FALSE)
    else()
        set(${result} TRUE)
    endif()

    set(QT_HAVE_LINK_ORDER_MATTERS "${${result}}" CACHE INTERNAL "Link order matters")

    set(${result} "${${result}}" PARENT_SCOPE)
endfunction()

# Sets _qt_link_order_matters flag for the target.
function(__qt_internal_set_link_order_matters target link_order_matters)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unable to set _qt_link_order_matters flag. ${target} is not a target.")
    endif()

    get_target_property(aliased_target ${target} ALIASED_TARGET)
    if(aliased_target)
        set(target "${aliased_target}")
    endif()

    if(link_order_matters)
        set(link_order_matters TRUE)
    else()
        set(link_order_matters FALSE)
    endif()
    set_target_properties(${target} PROPERTIES _qt_link_order_matters "${link_order_matters}")
endfunction()

# Function combines __qt_internal_static_link_order_public_test and
# __qt_internal_set_link_order_matters calls on Qt::Platform target.
function(__qt_internal_check_link_order_matters)
    __qt_internal_static_link_order_public_test(
         link_order_matters
    )
    __qt_internal_set_link_order_matters(
        ${QT_CMAKE_EXPORT_NAMESPACE}::Platform "${link_order_matters}"
    )

    if("${ARGC}" GREATER "0" AND NOT ARGV0 STREQUAL "")
        set(${ARGV0} ${link_order_matters} PARENT_SCOPE)
    endif()
endfunction()

# Constructs a TARGET_POLICY genex expression if the policy is available.
function(__qt_internal_get_cmp0099_genex_check result)
    if(POLICY CMP0099)
        set(${result} "$<BOOL:$<TARGET_POLICY:CMP0099>>" PARENT_SCOPE)
    else()
        set(${result} "$<BOOL:FALSE>" PARENT_SCOPE)
    endif()
endfunction()

function(__qt_internal_check_cmp0099_available)
    set(platform_target ${QT_CMAKE_EXPORT_NAMESPACE}::Platform)
    get_target_property(aliased_target ${platform_target} ALIASED_TARGET)
    if(aliased_target)
        set(platform_target "${aliased_target}")
    endif()

    __qt_internal_get_cmp0099_genex_check(cmp0099_check)
    set_target_properties(${platform_target} PROPERTIES
        _qt_cmp0099_policy_check "${cmp0099_check}"
    )

    set(result TRUE)
    if(NOT POLICY CMP0099)
        set(result FALSE)
    endif()

    if("${ARGC}" GREATER "0" AND NOT ARGV0 STREQUAL "")
        set(${ARGV0} ${result} PARENT_SCOPE)
    endif()
endfunction()

function(__qt_internal_process_dependency_object_libraries target)
    # The CMake versions greater than 3.21 take care about the order of object files in a
    # linker line, it's expected that all object files are located at the beginning of the linker
    # line.
    # So circular dependencies between static libraries and object files are resolved and no need
    # to call the finalizer code.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
        return()
    endif()
    get_target_property(processed ${target} _qt_object_libraries_finalizer_processed)
    if(processed)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_object_libraries_finalizer_processed TRUE)

    get_target_property(qt_link_order_matters
        ${QT_CMAKE_EXPORT_NAMESPACE}::Platform _qt_link_order_matters
    )
    __qt_internal_check_finalizer_mode(${target}
        use_finalizer_mode
        object_libraries
        DEFAULT_VALUE "${qt_link_order_matters}"
    )

    if(NOT use_finalizer_mode)
        return()
    endif()

    __qt_internal_collect_dependency_object_libraries(${target} objects)
    target_sources(${target} PRIVATE "${objects}")
endfunction()

function(__qt_internal_collect_dependency_object_libraries target out_var)
    set_property(GLOBAL PROPERTY _qt_processed_object_libraries "")

    __qt_internal_collect_object_libraries_recursively(object_libraries ${target} ${target})

    # Collect object libraries of plugins and plugin dependencies.
    __qt_internal_collect_plugin_targets_from_dependencies(${target} plugin_targets)
    __qt_internal_collect_dependency_plugin_object_libraries(${target}
        "${plugin_targets}"
        plugin_objects
    )

    set_property(GLOBAL PROPERTY _qt_processed_object_libraries "")
    __qt_internal_get_cmp0099_genex_check(cmp0099_check)

    list(REMOVE_DUPLICATES object_libraries)
    set(objects "")
    foreach(dep IN LISTS object_libraries)
        list(PREPEND objects "$<$<NOT:${cmp0099_check}>:$<TARGET_OBJECTS:${dep}>>")
    endforeach()

    set(${out_var} "${plugin_objects};${objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_dependency_plugin_object_libraries target plugin_targets out_var)
    __qt_internal_get_cmp0099_genex_check(cmp0099_check)
    set(plugin_objects "")
    foreach(plugin_target IN LISTS plugin_targets)
        __qt_internal_collect_object_libraries_recursively(plugin_object_libraries
            "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}"
            ${target}
        )
        __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

        foreach(plugin_object_library IN LISTS plugin_object_libraries)
            string(JOIN "" plugin_objects_genex
                "$<"
                    "$<AND:"
                        "$<NOT:${cmp0099_check}>,"
                        "${plugin_condition}"
                    ">"
                    ":$<TARGET_OBJECTS:${plugin_object_library}>"
                ">"
            )
            list(APPEND plugin_objects "${plugin_objects_genex}")
        endforeach()
    endforeach()
    set(${out_var} "${plugin_objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_object_libraries_recursively out_var target initial_target)
    get_property(processed_object_libraries GLOBAL PROPERTY _qt_processed_object_libraries)

    set(interface_libs "")
    set(libs "")
    if(NOT "${target}" STREQUAL "${initial_target}")
        get_target_property(interface_libs ${target} INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(type ${target} TYPE)
    if(NOT type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(libs ${target} LINK_LIBRARIES)
    endif()

    set(object_libraries "")
    foreach(lib IN LISTS libs interface_libs)
        # Extract possible target from exported LINK_ONLY dependencies.
        # This is super important for traversing backing library dependencies of qml plugins.
        if(lib MATCHES "^\\$<LINK_ONLY:(.*)>$")
            set(lib "${CMAKE_MATCH_1}")
        endif()
        if(TARGET ${lib})
            get_target_property(aliased_target ${lib} ALIASED_TARGET)
            if(aliased_target)
                set(lib ${aliased_target})
            endif()

            if(${lib} IN_LIST processed_object_libraries)
                continue()
            else()
                list(APPEND processed_object_libraries ${lib})
                set_property(GLOBAL APPEND PROPERTY _qt_processed_object_libraries ${lib})
            endif()

            get_target_property(is_qt_propagated_object_library ${lib}
                _is_qt_propagated_object_library
            )
            if(is_qt_propagated_object_library)
                list(APPEND object_libraries ${lib})
            else()
                __qt_internal_collect_object_libraries_recursively(next_level_object_libraries
                    ${lib}
                    ${initial_target}
                )
                list(APPEND object_libraries ${next_level_object_libraries})
            endif()
        endif()
    endforeach()
    set(${out_var} "${object_libraries}" PARENT_SCOPE)
endfunction()

function(__qt_internal_promote_target_to_global target)
    get_property(is_global TARGET ${target} PROPERTY IMPORTED_GLOBAL)
    if(NOT is_global)
        message(DEBUG "Promoting target to global: '${target}'")
        set_property(TARGET ${target} PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
endfunction()

function(__qt_internal_promote_target_to_global_checked target)
    # With CMake version 3.21 we use a different mechanism that allows us to promote all targets
    # within a scope.
    if(QT_PROMOTE_TO_GLOBAL_TARGETS AND CMAKE_VERSION VERSION_LESS 3.21)
        __qt_internal_promote_target_to_global(${target})
    endif()
endfunction()

function(__qt_internal_promote_targets_in_dir_scope_to_global)
    # IMPORTED_TARGETS got added in 3.21.
    if(CMAKE_VERSION VERSION_LESS 3.21)
        return()
    endif()

    get_directory_property(targets IMPORTED_TARGETS)
    foreach(target IN LISTS targets)
        __qt_internal_promote_target_to_global(${target})
    endforeach()
endfunction()

function(__qt_internal_promote_targets_in_dir_scope_to_global_checked)
    if(QT_PROMOTE_TO_GLOBAL_TARGETS)
        __qt_internal_promote_targets_in_dir_scope_to_global()
    endif()
endfunction()

# This function ends up being called multiple times as part of a find_package(Qt6Foo) call,
# due sub-packages depending on the Qt6 package. Ensure the finalizer is ran only once per
# directory scope.
function(__qt_internal_defer_promote_targets_in_dir_scope_to_global)
    get_directory_property(is_deferred _qt_promote_targets_is_deferred)
    if(NOT is_deferred)
        set_property(DIRECTORY PROPERTY _qt_promote_targets_is_deferred TRUE)

        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
            cmake_language(DEFER CALL __qt_internal_promote_targets_in_dir_scope_to_global_checked)
        endif()
    endif()
endfunction()

function(_qt_internal_set_up_static_runtime_library target)
    if(QT_FEATURE_static_runtime)
        if(MSVC)
            set_property(TARGET ${target} PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        elseif(MINGW)
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "EXECUTABLE")
                set(link_option PRIVATE)
            else()
                set(link_option INTERFACE)
            endif()
            if(CLANG)
                target_link_options(${target} ${link_option} "LINKER:-Bstatic")
            else()
                target_link_options(${target} ${link_option} "-static")
            endif()
        endif()
    endif()
endfunction()
