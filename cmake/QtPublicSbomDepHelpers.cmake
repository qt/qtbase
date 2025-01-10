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
    set(external_spdx_dependencies "")

    # Go through each direct linked lib.
    foreach(direct_lib IN LISTS all_direct_libraries)
        if(NOT TARGET "${direct_lib}")
            continue()
        endif()

        # Check for Qt-specific system library targets. These are marked via qt_find_package calls.
        get_target_property(is_system_library "${direct_lib}" _qt_internal_sbom_is_system_library)
        if(is_system_library)

            # We need to check if the dependency is a FindWrap dependency that points either to a
            # system library or a vendored / bundled library. We need to depend on whichever one
            # the FindWrap script points to.
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

                if(bundled_targets_found)
                    # If we handled a bundled target, we can move on to process the next direct_lib.
                    continue()
                endif()
            endif()

            if(NOT bundled_targets_found)
                # If we haven't found a bundled target, then it's a regular system library
                # dependency. Make sure to mark the system library as consumed, so that we later
                # generate an sbom for it.
                # Also fall through to the code that actually adds the dependency on the target.
                _qt_internal_append_to_cmake_property_without_duplicates(
                    _qt_internal_sbom_consumed_system_library_targets
                    "${direct_lib}"
                )
            endif()
        endif()

        # Get the spdx id of the dependency.
        _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" lib_spdx_id)
        if(NOT lib_spdx_id)
            message(DEBUG "Could not add target dependency on target ${direct_lib} "
                "because no spdx id for it could be found.")
            continue()
        endif()

        # Check if the target sbom is defined in an external document.

        _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
            OUT_VAR is_dependency_in_external_document
        )

        if(NOT is_dependency_in_external_document)
            # If the target is not in the external document, it must be one built as part of the
            # current project.
            list(APPEND spdx_dependencies "${lib_spdx_id}")
        else()
            # Refer to the package in the external document. This can be the case
            # in a top-level build, where a system library is reused across repos, or for any
            # regular dependency that was built as part of a different project.
            _qt_internal_sbom_add_external_target_dependency("${direct_lib}"
                extra_spdx_dependencies
            )
            if(extra_spdx_dependencies)
                list(APPEND external_spdx_dependencies ${extra_spdx_dependencies})
            endif()
        endif()
    endforeach()

    set(relationships "")
    # Keep the external dependencies first, so they are neatly ordered.
    foreach(dep_spdx_id IN LISTS external_spdx_dependencies spdx_dependencies)
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

    set(${arg_OUT_VAR} "${part_of_other_repo}" PARENT_SCOPE)
endfunction()

# Handles generating an external document reference SDPX element for each target package that is
# located in a different spdx document.
function(_qt_internal_sbom_add_external_target_dependency target out_spdx_dependencies)
    _qt_internal_sbom_get_spdx_id_for_target("${target}" dep_spdx_id)

    if(NOT dep_spdx_id)
        message(DEBUG "Could not add external target dependency on ${target} "
            "because no spdx id could be found")
        set(${out_spdx_dependencies} "" PARENT_SCOPE)
        return()
    endif()

    set(spdx_dependencies "")

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

        set(dependency "${external_document_ref}:${dep_spdx_id}")

        list(APPEND spdx_dependencies "${dependency}")

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
        message(AUTHOR_WARNING
            "Missing spdx document path for external target dependency: ${target}")
    endif()

    set(${out_spdx_dependencies} "${spdx_dependencies}" PARENT_SCOPE)
endfunction()
