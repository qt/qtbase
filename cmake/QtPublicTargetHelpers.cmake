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
    # TODO: The __qt_internal_collect_all_target_dependencies function is not cheap and called
    # multiple times in the finalizer jobs. It make sense to re-use its results.
    __qt_internal_collect_all_target_dependencies(${target} dep_targets)

    set(resource_objects "")
    foreach(dep IN LISTS dep_targets)
        get_target_property(dep_resource_targets ${dep} _qt_resource_object_libraries)
        foreach(resource_target IN LISTS dep_resource_targets)
            if(resource_target)
                list(PREPEND resource_objects "$<TARGET_OBJECTS:$<TARGET_NAME:${resource_target}>>")
            endif()
        endforeach()
    endforeach()

    set(${out_var} "${resource_objects}" PARENT_SCOPE)
endfunction()
