function(qt_internal_add_resource target resourceName)
    # Don't try to add resources when cross compiling, and the target is actually a host target
    # (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_add_resource" "" "PREFIX;LANG;BASE" "FILES" ${ARGN})

    _qt_internal_process_resource(${target} ${resourceName}
        PREFIX "${arg_PREFIX}"
        LANG "${arg_LANG}"
        BASE "${arg_BASE}"
        FILES ${arg_FILES}
        OUTPUT_TARGETS out_targets
   )

   if (out_targets)
        qt_install(TARGETS ${out_targets}
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
            DESTINATION ${INSTALL_LIBDIR}
        )

        qt_internal_record_rcc_object_files("${target}" "${out_targets}")
   endif()
endfunction()

function(qt_internal_record_rcc_object_files target resource_targets)
    foreach(out_target ${resource_targets})
        get_target_property(resource_name ${out_target} _qt_resource_name)
        if(NOT resource_name)
            continue()
        endif()
        if(QT_WILL_INSTALL)
            # Compute the install location of the rcc object file.
            # This is the relative path below the install destination (install_prefix/lib).
            # See CMake's computeInstallObjectDir function.
            set(object_file_name "qrc_${resource_name}.cpp${CMAKE_CXX_OUTPUT_EXTENSION}")
            qt_path_join(rcc_object_file_path
                "objects-$<CONFIG>" ${out_target} .rcc "${object_file_name}")
        else()
            # In a non-prefix build we use the object file paths right away.
            set(rcc_object_file_path $<TARGET_OBJECTS:$<TARGET_NAME:${out_target}>>)
        endif()
        set_property(TARGET ${target} APPEND PROPERTY _qt_rcc_objects "${rcc_object_file_path}")

        # Make sure that the target cpp files are compiled with the regular Qt internal compile
        # flags, needed for building iOS apps with qmake where bitcode is involved.
        target_link_libraries("${out_target}" PRIVATE Qt::PlatformModuleInternal)
    endforeach()
endfunction()
