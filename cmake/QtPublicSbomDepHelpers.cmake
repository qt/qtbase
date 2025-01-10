# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Walks a target's direct dependencies and assembles a list of relationships between the packages
# of the target dependencies.
# Currently handles various Qt targets and system libraries.
function(_qt_internal_sbom_handle_target_dependencies target)
    set(opt_args "")
    set(single_args
        SPDX_ID
        OUT_RELATIONSHIPS
    )
    set(multi_args
        LIBRARIES
        PUBLIC_LIBRARIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()
    set(package_spdx_id "${arg_SPDX_ID}")


    set(libraries "")
    if(arg_LIBRARIES)
        list(APPEND libraries "${arg_LIBRARIES}")
    endif()

    get_target_property(extend_libraries "${target}" _qt_extend_target_libraries)
    if(extend_libraries)
        list(APPEND libraries ${extend_libraries})
    endif()

    get_target_property(target_type ${target} TYPE)
    set(valid_target_types
        EXECUTABLE
        SHARED_LIBRARY
        MODULE_LIBRARY
        STATIC_LIBRARY
        OBJECT_LIBRARY
    )
    if(target_type IN_LIST valid_target_types)
        get_target_property(link_libraries "${target}" LINK_LIBRARIES)
        if(link_libraries)
            list(APPEND libraries ${link_libraries})
        endif()
    endif()

    set(public_libraries "")
    if(arg_PUBLIC_LIBRARIES)
        list(APPEND public_libraries "${arg_PUBLIC_LIBRARIES}")
    endif()

    get_target_property(extend_public_libraries "${target}" _qt_extend_target_public_libraries)
    if(extend_public_libraries)
        list(APPEND public_libraries ${extend_public_libraries})
    endif()

    set(sbom_dependencies "")
    if(arg_SBOM_DEPENDENCIES)
        list(APPEND sbom_dependencies "${arg_SBOM_DEPENDENCIES}")
    endif()

    get_target_property(extend_sbom_dependencies "${target}" _qt_extend_target_sbom_dependencies)
    if(extend_sbom_dependencies)
        list(APPEND sbom_dependencies ${extend_sbom_dependencies})
    endif()

    list(REMOVE_DUPLICATES libraries)
    list(REMOVE_DUPLICATES public_libraries)
    list(REMOVE_DUPLICATES sbom_dependencies)

    set(all_direct_libraries ${libraries} ${public_libraries} ${sbom_dependencies})
    list(REMOVE_DUPLICATES all_direct_libraries)

    set(spdx_dependencies "")
    set(relationships "")

    # Go through each direct linked lib.
    foreach(direct_lib IN LISTS all_direct_libraries)
        if(NOT TARGET "${direct_lib}")
            continue()
        endif()

        # Some targets are Qt modules, even though they are not prefixed with Qt::, targets
        # like Bootstrap and QtLibraryInfo. We use the property to differentiate them.
        get_target_property(is_marked_as_qt_module "${direct_lib}" _qt_sbom_is_qt_module)

        # Custom sbom targets created by _qt_internal_create_sbom_target are always imported, so we
        # need to differentiate them via this property.
        get_target_property(is_custom_sbom_target "${direct_lib}" _qt_sbom_is_custom_sbom_target)

        if("${direct_lib}" MATCHES "^(Qt::.*)|(${QT_CMAKE_EXPORT_NAMESPACE}::.*)")
            set(is_qt_prefixed TRUE)
        else()
            set(is_qt_prefixed FALSE)
        endif()

        # is_qt_dependency is not strictly only a qt dependency, it applies to custom sbom
        # targets as well. But I'm having a hard time to come up with a better name.
        if(is_marked_as_qt_module OR is_custom_sbom_target OR is_qt_prefixed)
            set(is_qt_dependency TRUE)
        else()
            set(is_qt_dependency FALSE)
        endif()

        # Regular Qt dependency, depend on the relevant package, either within the current
        # document or via an external document.
        if(is_qt_dependency)
            _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
                OUT_VAR is_dependency_in_external_document
            )

            if(is_dependency_in_external_document)
                # External document case.
                _qt_internal_sbom_add_external_target_dependency(
                    "${package_spdx_id}" "${direct_lib}"
                    extra_spdx_dependencies
                    extra_spdx_relationships
                )
                if(extra_spdx_dependencies)
                    list(APPEND spdx_dependencies "${extra_spdx_dependencies}")
                endif()
                if(extra_spdx_relationships)
                    list(APPEND relationships "${extra_spdx_relationships}")
                endif()
            else()
                # Dependency is part of current repo build.
                _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" dep_spdx_id)
                if(dep_spdx_id)
                    list(APPEND spdx_dependencies "${dep_spdx_id}")
                else()
                    message(DEBUG "Could not add target dependency on ${direct_lib} "
                        "because no spdx id could be found")
                endif()
            endif()
        else()
            # If it's not a Qt dependency, then it's most likely a 3rd party dependency.
            # If we are looking at a FindWrap dependency, we need to depend on either
            # the system or vendored lib, whichever one the FindWrap script points to.
            # If we are looking at a non-Wrap dependency, it's 99% a system lib.
            __qt_internal_walk_libs(
                "${direct_lib}"
                lib_walked_targets
                _discarded_out_var
                "sbom_targets"
                "collect_targets")

            # Detect if we are dealing with a vendored / bundled lib.
            set(bundled_targets_found FALSE)
            if(lib_walked_targets)
                foreach(lib_walked_target IN LISTS lib_walked_targets)
                    get_target_property(is_3rdparty_bundled_lib
                        "${lib_walked_target}" _qt_module_is_3rdparty_library)
                    _qt_internal_sbom_get_spdx_id_for_target("${lib_walked_target}" lib_spdx_id)

                    # Add a dependency on the vendored lib instead of the Wrap target.
                    if(is_3rdparty_bundled_lib AND lib_spdx_id)
                        list(APPEND spdx_dependencies "${lib_spdx_id}")
                        set(bundled_targets_found TRUE)
                    endif()
                endforeach()
            endif()

            # If no bundled libs were found as a result of walking the Wrap lib, we consider this
            # a system lib, and add a dependency on it directly.
            if(NOT bundled_targets_found)
                _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" lib_spdx_id)
                _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
                    SYSTEM_LIBRARY
                    OUT_VAR is_dependency_in_external_document
                )

                if(lib_spdx_id)
                    if(NOT is_dependency_in_external_document)
                        list(APPEND spdx_dependencies "${lib_spdx_id}")

                        # Mark the system library is used, so that we later generate an sbom for it.
                        _qt_internal_append_to_cmake_property_without_duplicates(
                            _qt_internal_sbom_consumed_system_library_targets
                            "${direct_lib}"
                        )
                    else()
                        # Refer to the package in the external document. This can be the case
                        # in a top-level build, where a system library is reused across repos.
                        _qt_internal_sbom_add_external_target_dependency(
                            "${package_spdx_id}" "${direct_lib}"
                            extra_spdx_dependencies
                            extra_spdx_relationships
                        )
                        if(extra_spdx_dependencies)
                            list(APPEND spdx_dependencies "${extra_spdx_dependencies}")
                        endif()
                        if(extra_spdx_relationships)
                            list(APPEND relationships "${extra_spdx_relationships}")
                        endif()
                    endif()
                else()
                    message(DEBUG "Could not add target dependency on system library ${direct_lib} "
                        "because no spdx id could be found")
                endif()
            endif()
        endif()
    endforeach()

    foreach(dep_spdx_id IN LISTS spdx_dependencies)
        set(relationship
            "${package_spdx_id} DEPENDS_ON ${dep_spdx_id}"
        )
        list(APPEND relationships "${relationship}")
    endforeach()

    set(${arg_OUT_RELATIONSHIPS} "${relationships}" PARENT_SCOPE)
endfunction()

# Checks whether the current target will have its sbom generated into the current repo sbom
# document, or whether it is present in an external sbom document.
function(_qt_internal_sbom_is_external_target_dependency target)
    set(opt_args
        SYSTEM_LIBRARY
    )
    set(single_args
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(is_imported "${target}" IMPORTED)
    get_target_property(is_custom_sbom_target "${target}" _qt_sbom_is_custom_sbom_target)

    _qt_internal_sbom_get_root_project_name_lower_case(current_repo_project_name)
    get_property(target_repo_project_name TARGET ${target}
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    if(NOT "${target_repo_project_name}" STREQUAL ""
            AND NOT "${target_repo_project_name}" STREQUAL "${current_repo_project_name}")
        set(part_of_other_repo TRUE)
    else()
        set(part_of_other_repo FALSE)
    endif()

    # A target is in an external document if
    # 1) it is imported, and not a custom sbom target, and not a system library
    # 2) it was created as part of another repo in a top-level build
    if((is_imported AND NOT is_custom_sbom_target AND NOT arg_SYSTEM_LIBRARY)
            OR part_of_other_repo)
        set(is_dependency_in_external_document TRUE)
    else()
        set(is_dependency_in_external_document FALSE)
    endif()

    set(${arg_OUT_VAR} "${is_dependency_in_external_document}" PARENT_SCOPE)
endfunction()

# Handles generating an external document reference SDPX element for each target package that is
# located in a different spdx document.
function(_qt_internal_sbom_add_external_target_dependency
        current_package_spdx_id
        target_dep
        out_spdx_dependencies
        out_spdx_relationships
    )
    set(target "${target_dep}")

    _qt_internal_sbom_get_spdx_id_for_target("${target}" dep_spdx_id)

    if(NOT dep_spdx_id)
        message(DEBUG "Could not add external target dependency on ${target} "
            "because no spdx id could be found")
        set(${out_spdx_dependencies} "" PARENT_SCOPE)
        set(${out_spdx_relationships} "" PARENT_SCOPE)
        return()
    endif()

    set(spdx_dependencies "")
    set(spdx_relationships "")

    # Get the external document path and the repo it belongs to for the given target.
    get_property(relative_installed_repo_document_path TARGET ${target}
        PROPERTY _qt_sbom_spdx_relative_installed_repo_document_path)

    get_property(project_name_lowercase TARGET ${target}
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    if(relative_installed_repo_document_path AND project_name_lowercase)
        _qt_internal_sbom_get_external_document_ref_spdx_id(
            "${project_name_lowercase}" external_document_ref)

        get_cmake_property(known_external_document
            _qt_known_external_documents_${external_document_ref})

        set(relationship
            "${current_package_spdx_id} DEPENDS_ON ${external_document_ref}:${dep_spdx_id}")

        list(APPEND spdx_relationships "${relationship}")

        # Only add a reference to the external document package, if we haven't done so already.
        if(NOT known_external_document)
            set(install_prefixes "")

            get_cmake_property(install_prefix _qt_internal_sbom_install_prefix)
            list(APPEND install_prefixes "${install_prefix}")

            set(external_document "${relative_installed_repo_document_path}")

            _qt_internal_sbom_generate_add_external_reference(
                EXTERNAL_DOCUMENT_FILE_PATH "${external_document}"
                EXTERNAL_DOCUMENT_INSTALL_PREFIXES ${install_prefixes}
                EXTERNAL_DOCUMENT_SPDX_ID "${external_document_ref}"
            )

            set_property(GLOBAL PROPERTY
                _qt_known_external_documents_${external_document_ref} TRUE)
            set_property(GLOBAL APPEND PROPERTY
                _qt_known_external_documents "${external_document_ref}")
        endif()
    else()
        message(WARNING "Missing spdx document path for external ref: "
            "package_name_for_spdx_id ${package_name_for_spdx_id} direct_lib ${direct_lib}")
    endif()

    set(${out_spdx_dependencies} "${spdx_dependencies}" PARENT_SCOPE)
    set(${out_spdx_relationships} "${spdx_relationships}" PARENT_SCOPE)
endfunction()
