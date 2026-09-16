# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

__qt_internal_cmake_include_guard(GLOBAL GUARD_KEY "QtPublicSbomSystemDepHelpers")

# Records information about a system library target, usually due to a qt_find_package call.
# This information is later used to generate packages for the system libraries, but only after
# confirming that the library was used (linked) into any of the Qt targets.
function(_qt_internal_sbom_record_system_library_usage target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        TYPE # deprecated
        SBOM_ENTITY_TYPE
        PACKAGE_VERSION
        FRIENDLY_PACKAGE_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_map_sbom_entity_type(sbom_entity_type ${ARGN})

    if(NOT sbom_entity_type)
        message(FATAL_ERROR "SBOM_ENTITY_TYPE is empty for target '${target}', but it must be set")
    endif()

    # A package might be looked up more than once, make sure to record it once.
    get_property(already_recorded GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target})

    if(already_recorded)
        return()
    endif()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target} TRUE)

    # Defer spdx id creation until _qt_internal_sbom_begin_project is called, so we know the
    # project name. The project name is used in the package infix generation of the system library,
    # but _qt_internal_sbom_record_system_library_usage might be called before sbom generation
    # has started, e.g. during _qt_internal_find_third_party_dependencies.
    set(spdx_options
        ${target}
        SBOM_ENTITY_TYPE "${sbom_entity_type}"
        PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}"
    )

    get_cmake_property(sbom_repo_begin_called _qt_internal_sbom_repo_begin_called)
    if(sbom_repo_begin_called AND TARGET "${target}")
        _qt_internal_sbom_record_system_library_spdx_id(${target} ${spdx_options})
    else()
        set_property(GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target} "${spdx_options}")
    endif()

    # Resolve the attribution file now, while the target is still in scope. The sbom packages are
    # only created in _qt_internal_sbom_end_project, and an imported target is directory scoped
    # unless it was promoted to GLOBAL, which does not happen in a user project.
    set(attribution_args "")
    if(TARGET "${target}")
        _qt_internal_sbom_find_system_library_attribution_file(attribution_file
            "${target}" "${arg_FRIENDLY_PACKAGE_NAME}")
        if(attribution_file)
            set(attribution_args
                USE_ATTRIBUTION_FILES
                ATTRIBUTION_FILE_PATHS "${attribution_file}"
            )
        endif()
    endif()

    # Defer sbom info creation until we detect usage of the system library (whether the library is
    # linked into any other target).
    set_property(GLOBAL APPEND PROPERTY
        _qt_internal_sbom_recorded_system_library_targets "${target}")
    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_options_${target} "${ARGN}" ${attribution_args})
endfunction()

# Helper to record spdx ids of all system library targets that were found so far.
function(_qt_internal_sbom_record_system_library_spdx_ids)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)

    if(NOT recorded_targets)
        return()
    endif()

    foreach(target IN LISTS recorded_targets)
        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target})

        # qt_find_package PROVIDED_TARGETS might refer to non-existent targets in certain cases,
        # like zstd::libzstd_shared for qt_find_package(WrapZSTD), because we are not sure what
        # kind of zstd build was done. Make sure to check if the target exists before recording it.
        if(TARGET "${target}")
            set(target_unaliased "${target}")
            _qt_internal_dealias_target(target_unaliased)

            _qt_internal_sbom_record_system_library_spdx_id(${target_unaliased} ${args})
        else()
            message(DEBUG
                "Skipping recording system library for SBOM because target does not exist: "
                " ${target}")
        endif()
    endforeach()
endfunction()

# Helper to record the spdx id of a system library target.
function(_qt_internal_sbom_record_system_library_spdx_id target)
    # Save the spdx id before the sbom info is added, so we can refer to it in relationships.
    _qt_internal_sbom_record_target_spdx_id(${ARGN} OUT_VAR package_spdx_id)

    if(NOT package_spdx_id)
        message(FATAL_ERROR "Could not generate spdx id for system library target: ${target}")
    endif()

    set_target_properties("${target}" PROPERTIES _qt_internal_sbom_is_system_library TRUE)

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_package_${target} "${package_spdx_id}")
endfunction()

# A candidate attribution file might exist but be empty, e.g. when provisioning was interrupted
# between truncating the file and writing its contents. Treat that the same as a missing file,
# rather than letting the empty-file FATAL_ERROR in _qt_internal_sbom_read_qt_attribution abort
# the whole configure.
function(_qt_internal_sbom_is_usable_attribution_file out_var path)
    set(${out_var} FALSE PARENT_SCOPE)
    if(NOT EXISTS "${path}")
        return()
    endif()
    file(SIZE "${path}" size)
    if(size GREATER 0)
        set(${out_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Returns the attribution file of the vcpkg port that owns the given installed file, using the
# index written by coin/provisioning/common/shared/vcpkg_generate_attributions.py.
function(_qt_internal_sbom_attribution_file_for_vcpkg_file out_var file_path)
    set(${out_var} "" PARENT_SCOPE)

    get_filename_component(real_path "${file_path}" REALPATH)

    # <prefix>/lib/libfoo.a -> <prefix>
    get_filename_component(containing_dir "${real_path}" DIRECTORY)
    get_filename_component(prefix "${containing_dir}" DIRECTORY)

    set(index_path "${prefix}/share/qt_vcpkg_ports.json")
    if(NOT EXISTS "${index_path}")
        message(DEBUG "No vcpkg port index at ${index_path}, for library: ${real_path}")
        return()
    endif()

    file(RELATIVE_PATH relative_path "${prefix}" "${real_path}")
    file(READ "${index_path}" index_content)

    string(JSON port_name ERROR_VARIABLE json_error GET "${index_content}" "${relative_path}")
    if(json_error OR NOT port_name)
        message(DEBUG
            "No vcpkg port owns ${relative_path} according to ${index_path}: ${json_error}")
        return()
    endif()

    set(candidate "${prefix}/share/${port_name}/qt_attribution.json")
    _qt_internal_sbom_is_usable_attribution_file(is_usable "${candidate}")
    if(is_usable)
        set(${out_var} "${candidate}" PARENT_SCOPE)
    else()
        message(DEBUG "No usable attribution file for vcpkg port ${port_name}: ${candidate}")
    endif()
endfunction()

# Walks a target and its link interface looking for an imported library that lives in a vcpkg
# prefix, and returns the attribution file of the port that owns it.
function(_qt_internal_sbom_find_vcpkg_attribution_file out_var target)
    set(${out_var} "" PARENT_SCOPE)

    # A FindWrap target points at whichever library the wrapped package resolved to. Use the same
    # helper and dict that the sbom dependency handling uses to answer that question, so that
    # genex wrapped entries like $<LINK_ONLY:Foo> are unwrapped rather than skipped.
    __qt_internal_walk_libs(
        "${target}"
        walked_targets
        _discarded_out_var
        "sbom_targets"
        "collect_targets")

    # The walk returns the link closure, but the target itself might be the imported library.
    foreach(current "${target}" ${walked_targets})
        if(NOT TARGET "${current}")
            continue()
        endif()

        _qt_internal_dealias_target(current)

        get_target_property(is_imported "${current}" IMPORTED)
        if(NOT is_imported)
            continue()
        endif()

        get_target_property(location "${current}" IMPORTED_LOCATION)
        if(NOT location)
            get_target_property(configurations "${current}" IMPORTED_CONFIGURATIONS)
            foreach(config IN LISTS configurations)
                get_target_property(location "${current}" IMPORTED_LOCATION_${config})
                if(location)
                    break()
                endif()
            endforeach()
        endif()

        if(NOT location)
            continue()
        endif()

        _qt_internal_sbom_attribution_file_for_vcpkg_file(candidate "${location}")
        if(candidate)
            set(${out_var} "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    message(DEBUG "No vcpkg attribution file in the link closure of target: ${target}")
endfunction()

# Finds a generated qt_attribution.json belonging to a system library target, if there is one.
function(_qt_internal_sbom_find_system_library_attribution_file out_var target package_name)
    set(${out_var} "" PARENT_SCOPE)

    # A config mode find points <Package>_DIR straight at the port directory.
    if(package_name AND DEFINED ${package_name}_DIR)
        set(candidate "${${package_name}_DIR}/qt_attribution.json")
        _qt_internal_sbom_is_usable_attribution_file(is_usable "${candidate}")
        if(is_usable)
            set(${out_var} "${candidate}" PARENT_SCOPE)
            return()
        endif()
        message(DEBUG "No usable attribution file next to ${package_name}_DIR: ${candidate}")
    endif()

    # Qt finds most of these in MODULE mode via its own FindWrap*.cmake modules, which set no
    # <Package>_DIR, so fall back to the imported library location.
    _qt_internal_sbom_find_vcpkg_attribution_file(candidate "${target}")
    if(candidate)
        set(${out_var} "${candidate}" PARENT_SCOPE)
    endif()
endfunction()

# Goes through the list of consumed system libraries (those that were linked in) and creates
# sbom packages for them.
# Uses information from recorded system libraries (calls to qt_find_package).
function(_qt_internal_sbom_add_recorded_system_libraries)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)
    get_property(consumed_targets GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets)

    set(unconsumed_targets "${recorded_targets}")
    set(generated_package_names "")

    message(DEBUG
        "System libraries that were marked consumed "
        "(some target linked to them): ${consumed_targets}")
    message(DEBUG
        "System libraries that were recorded "
        "(they were marked with qt_find_package()): ${recorded_targets}")

    foreach(target IN LISTS consumed_targets)
        # Some system targets like qtspeech SpeechDispatcher::SpeechDispatcher might be aliased,
        # and we can't set properties on them, so unalias the target name.
        set(target_original "${target}")
        _qt_internal_dealias_target(target)

        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_options_${target})
        get_property(package_name GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_package_${target})

        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_target_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_options_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_package_${target} "")

        # Guard against generating a package multiple times. Can happen when multiple targets belong
        # to the same package.
        if(sbom_generated_${package_name})
            continue()
        endif()

        # Automatic system library sbom recording happens at project root source dir scope, which
        # means it might accidentally pick up a qt_attribution.json file from the project root,
        # that is not intended to be use for system libraries.
        # For now, explicitly disable using the root attribution file.
        list(APPEND args NO_CURRENT_DIR_ATTRIBUTION)

        list(APPEND generated_package_names "${package_name}")
        set(sbom_generated_${package_name} TRUE)

        _qt_internal_extend_sbom(${target} ${args})
        _qt_internal_finalize_sbom(${target})

        list(REMOVE_ITEM unconsumed_targets "${target_original}")
    endforeach()

    message(DEBUG "System libraries that were recorded, but not consumed: ${unconsumed_targets}")
    message(DEBUG "Generated SBOMs for the following system packages: ${generated_package_names}")

    # Clean up, before configuring next repo project.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets "")
endfunction()
