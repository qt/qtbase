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

function(__qt_internal_process_dependency_resource_objects target)
    get_target_property(processed ${target} _qt_resource_object_finalizers_processed)
    if(processed)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_resource_object_finalizers_processed TRUE)

    __qt_internal_check_finalizer_mode(${target} use_finalizer_mode resource_objects)
    if(NOT use_finalizer_mode)
        return()
    endif()

    __qt_internal_collect_dependency_resource_objects(${target} resource_objects)
    target_sources(${target} PRIVATE "${resource_objects}")
endfunction()

function(__qt_internal_collect_dependency_resource_objects target out_var)
    set_property(GLOBAL PROPERTY _qt_resource_processed_targets "")

    __qt_internal_collect_resource_objects_recursively(resource_targets ${target} ${target})

    # Collect resource objects of plugins and plugin dependencies.
    __qt_internal_collect_plugin_targets_from_dependencies(${target} plugin_targets)
    __qt_internal_collect_dependency_plugin_resource_objects(${target}
        "${plugin_targets}"
        plugin_resource_objects
    )

    set_property(GLOBAL PROPERTY _qt_resource_processed_targets "")

    list(REMOVE_DUPLICATES resource_targets)
    set(resource_objects "")
    foreach(dep IN LISTS resource_targets)
        list(PREPEND resource_objects "$<TARGET_OBJECTS:${dep}>")
    endforeach()

    set(${out_var} "${plugin_resource_objects};${resource_objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_dependency_plugin_resource_objects target plugin_targets out_var)
    set(plugin_resource_objects "")
    foreach(plugin_target IN LISTS plugin_targets)
        __qt_internal_collect_resource_objects_recursively(plugin_resource_targets
            "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}"
            ${target}
        )
        __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

        foreach(plugin_resource_target IN LISTS plugin_resource_targets)
            list(APPEND plugin_resource_objects
                "$<${plugin_condition}:$<TARGET_OBJECTS:${plugin_resource_target}>>"
            )
        endforeach()
    endforeach()
    set(${out_var} "${plugin_resource_objects}" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_resource_objects_recursively out_var target initial_target)
    get_property(resource_processed_targets GLOBAL PROPERTY _qt_resource_processed_targets)

    set(interface_libs "")
    set(libs "")
    if(NOT "${target}" STREQUAL "${initial_target}")
        get_target_property(interface_libs ${target} INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(type ${target} TYPE)
    if(NOT type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(libs ${target} LINK_LIBRARIES)
    endif()

    set(resource_targets "")
    foreach(lib IN LISTS libs interface_libs)
        if(TARGET ${lib})
            get_target_property(aliased_target ${lib} ALIASED_TARGET)
            if(aliased_target)
                set(lib ${aliased_target})
            endif()

            if(${lib} IN_LIST resource_processed_targets)
                continue()
            else()
                list(APPEND resource_processed_targets ${lib})
                set_property(GLOBAL APPEND PROPERTY _qt_resource_processed_targets ${lib})
            endif()

            get_target_property(is_qt_resource ${lib} _is_qt_resource_target)
            if(is_qt_resource)
                list(APPEND resource_targets ${lib})
            else()
                __qt_internal_collect_resource_objects_recursively(next_level_resources
                    ${lib}
                    ${initial_target}
                )
                list(APPEND resource_targets ${next_level_resources})
            endif()
        endif()
    endforeach()
    set(${out_var} "${resource_targets}" PARENT_SCOPE)
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
