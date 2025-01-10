# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Parse purl arguments for a specific purl variant, e.g. for parsing all values of arg_PURL_QT_ARGS.
# arguments_var_name is the variable name that contains the args.
macro(_qt_internal_sbom_parse_purl_variant_options prefix arguments_var_name)
    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    cmake_parse_arguments(arg "${purl_opt_args}" "${purl_single_args}" "${purl_multi_args}"
        ${${arguments_var_name}})
    _qt_internal_validate_all_args_are_parsed(arg)
endmacro()

# Returns a vcs url where for purls where qt entities of the current repo are hosted.
function(_qt_internal_sbom_get_qt_entity_vcs_url target)
    set(opt_args "")
    set(single_args
        REPO_NAME
        VERSION
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_REPO_NAME)
        message(FATAL_ERROR "REPO_NAME must be set")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    set(version_part "")
    if(arg_VERSION)
        set(version_part "@${arg_VERSION}")
    endif()

    set(vcs_url "https://code.qt.io/qt/${arg_REPO_NAME}.git${version_part}")
    set(${arg_OUT_VAR} "${vcs_url}" PARENT_SCOPE)
endfunction()

# Returns a relative path to the source where the target was created, to be embedded into a
# mirror purl as a subpath.
function(_qt_internal_sbom_get_qt_entity_repo_source_dir target)
    set(opt_args "")
    set(single_args
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    get_target_property(repo_source_dir "${target}" SOURCE_DIR)

    # Get the path relative to the PROJECT_SOURCE_DIR
    file(RELATIVE_PATH relative_repo_source_dir "${PROJECT_SOURCE_DIR}" "${repo_source_dir}")

    set(sub_path "${relative_repo_source_dir}")
    set(${arg_OUT_VAR} "${sub_path}" PARENT_SCOPE)
endfunction()

# Handles purl arguments specified to functions like qt_internal_add_sbom.
# Currently accepts arguments for 3 variants of purls, each of which will generate a separate purl.
# If no arguments are specified, for qt entity types, default values will be chosen.
#
# Purl variants:
# - PURL_QT_ARGS
#       args to override Qt's generic purl for Qt modules or patched 3rd party libs
#       defaults to something like pkg:generic/TheQtCompany/${repo_name}-${target}@SHA1
# - PURL_MIRROR_ARGS
#       args to override Qt's mirror purl, which is hosted on github
#       defaults to something like pkg:github/qt/${repo_name}@SHA1
# - PURL_3RDPARTY_UPSTREAM_ARGS
#       args to specify a purl pointing to an upstream repo, usually to github or another forge
#       no defaults, but could look like: pkg:github/harfbuzz/harfbuzz@v8.5.0
# Example values for harfbuzz:
#     PURL_3RDPARTY_UPSTREAM_ARGS
#         PURL_TYPE "github"
#         PURL_NAMESPACE "harfbuzz"
#         PURL_NAME "harfbuzz"
#         PURL_VERSION "v8.5.0" # tag
function(_qt_internal_sbom_handle_purl_values target)
    _qt_internal_get_sbom_purl_handling_options(opt_args single_args multi_args)
    list(APPEND single_args OUT_VAR)

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    # List of purl variants to process.
    set(purl_variants "")

    _qt_internal_sbom_get_git_version_vars()

    set(third_party_types
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
    )

    if(arg_IS_QT_ENTITY_TYPE)
        # Qt entities have two purls by default, a QT generic one and a MIRROR hosted on github.
        list(APPEND purl_variants MIRROR QT)
    elseif(arg_TYPE IN_LIST third_party_types)
        # Third party libraries vendored in Qt also have at least two purls, like regular Qt
        # libraries, but might also have an upstream one.

        # The order in which the purls are generated matters for tools that consume the SBOM. Some
        # tools can only handle one PURL per package, so the first one should be the important one.
        # For now, I deem that the upstream one if present. Otherwise the github mirror.
        if(arg_PURL_3RDPARTY_UPSTREAM_ARGS)
            list(APPEND purl_variants 3RDPARTY_UPSTREAM)
        endif()

        list(APPEND purl_variants MIRROR QT)
    else()
        # If handling another entity type, handle based on whether any of the purl arguments are
        # set.
        set(known_purl_variants QT MIRROR 3RDPARTY_UPSTREAM)
        foreach(known_purl_variant IN LISTS known_purl_variants)
            if(arg_PURL_${known_purl_variant}_ARGS)
                list(APPEND purl_variants ${known_purl_variant})
            endif()
        endforeach()
    endif()

    if(arg_IS_QT_ENTITY_TYPE
            OR arg_TYPE STREQUAL "QT_THIRD_PARTY_MODULE"
            OR arg_TYPE STREQUAL "QT_THIRD_PARTY_SOURCES"
        )
        set(is_qt_purl_entity_type TRUE)
    else()
        set(is_qt_purl_entity_type FALSE)
    endif()

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    set(project_package_options "")

    foreach(purl_variant IN LISTS purl_variants)
        # Clear previous values.
        foreach(option_name IN LISTS purl_opt_args purl_single_args purl_multi_args)
            unset(arg_${option_name})
        endforeach()

        _qt_internal_sbom_parse_purl_variant_options(arg arg_PURL_${purl_variant}_ARGS)

        # Check if custom purl args were specified.
        set(purl_args_available FALSE)
        if(arg_PURL_${purl_variant}_ARGS)
            set(purl_args_available TRUE)
        endif()

        # We want to create a purl either if it's one of Qt's entities and one of it's default
        # purl types, or if custom args were specified.
        set(consider_purl_processing FALSE)
        if((purl_args_available OR is_qt_purl_entity_type) AND NOT arg_NO_PURL)
            set(consider_purl_processing TRUE)
        endif()

        if(consider_purl_processing)
            set(purl_args "")

            # Override the purl version with the package version.
            if(arg_PURL_USE_PACKAGE_VERSION AND arg_VERSION)
                set(arg_PURL_VERSION "${arg_VERSION}")
            endif()

            # Append a vcs_url to the qualifiers if specified.
            if(arg_PURL_VCS_URL)
                list(APPEND arg_PURL_QUALIFIERS "vcs_url=${arg_PURL_VCS_URL}")
            endif()

            _qt_internal_forward_function_args(
                FORWARD_APPEND
                FORWARD_PREFIX arg
                FORWARD_OUT_VAR purl_args
                FORWARD_OPTIONS
                    ${purl_opt_args}
                FORWARD_SINGLE
                    ${purl_single_args}
                FORWARD_MULTI
                    ${purl_multi_args}
            )

            # Qt entity types get special treatment purl.
            if(is_qt_purl_entity_type AND NOT arg_NO_DEFAULT_QT_PURL AND
                    (purl_variant STREQUAL "QT" OR purl_variant STREQUAL "MIRROR"))
                _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

                # Add a vcs_url to the generic QT variant.
                if(purl_variant STREQUAL "QT")
                    set(entity_vcs_url_version_option "")
                    # Can be empty.
                    if(QT_SBOM_GIT_HASH_SHORT)
                        set(entity_vcs_url_version_option VERSION "${QT_SBOM_GIT_HASH_SHORT}")
                    endif()

                    _qt_internal_sbom_get_qt_entity_vcs_url(${target}
                        REPO_NAME "${repo_project_name_lowercase}"
                        ${entity_vcs_url_version_option}
                        OUT_VAR vcs_url)
                    list(APPEND purl_args PURL_QUALIFIERS "vcs_url=${vcs_url}")
                endif()

                # Add the subdirectory path where the target was created as a custom qualifier.
                _qt_internal_sbom_get_qt_entity_repo_source_dir(${target} OUT_VAR sub_path)
                if(sub_path)
                    list(APPEND purl_args PURL_SUBPATH "${sub_path}")
                endif()

                # Add the target name as a custom qualifer.
                list(APPEND purl_args PURL_QUALIFIERS "library_name=${target}")

                # Can be empty.
                if(QT_SBOM_GIT_HASH_SHORT)
                    list(APPEND purl_args VERSION "${QT_SBOM_GIT_HASH_SHORT}")
                endif()

                # Get purl args the Qt entity type, taking into account defaults.
                _qt_internal_sbom_get_qt_entity_purl_args(${target}
                    NAME "${repo_project_name_lowercase}-${target}"
                    REPO_NAME "${repo_project_name_lowercase}"
                    SUPPLIER "${arg_SUPPLIER}"
                    PURL_VARIANT "${purl_variant}"
                    ${purl_args}
                    OUT_VAR purl_args
                )
            endif()

            _qt_internal_sbom_assemble_purl(${target}
                ${purl_args}
                OUT_VAR package_manager_external_ref
            )
            list(APPEND project_package_options ${package_manager_external_ref})
        endif()
    endforeach()

    set(direct_values
        PURL_QT_VALUES
        PURL_MIRROR_VALUES
        PURL_3RDPARTY_UPSTREAM_VALUES
    )

    foreach(direct_value IN LISTS direct_values)
        if(arg_${direct_value})
            set(direct_values_per_type "")
            foreach(direct_value IN LISTS arg_${direct_value})
                _qt_internal_sbom_get_purl_value_extref(
                    VALUE "${direct_value}" OUT_VAR package_manager_external_ref)

                list(APPEND direct_values_per_type ${package_manager_external_ref})
            endforeach()
            # The order in which the purls are generated, matters for tools that consume the SBOM.
            # Some tools can only handle one PURL per package, so the first one should be the
            # important one.
            # For now, I deem that the directly specified ones (probably via a qt_attribution.json
            # file) are the more important ones. So we prepend them.
            list(PREPEND project_package_options ${direct_values_per_type})
        endif()
    endforeach()

    set(${arg_OUT_VAR} "${project_package_options}" PARENT_SCOPE)
endfunction()

# Gets a list of arguments to pass to _qt_internal_sbom_assemble_purl when handling a Qt entity
# type. The purl for Qt entity types have Qt-specific defaults, but can be overridden per purl
# component.
# The arguments are saved in OUT_VAR.
function(_qt_internal_sbom_get_qt_entity_purl_args target)
    set(opt_args "")
    set(single_args
        NAME
        REPO_NAME
        SUPPLIER
        VERSION
        PURL_VARIANT
        OUT_VAR
    )
    set(multi_args "")

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)
    list(APPEND opt_args ${purl_opt_args})
    list(APPEND single_args ${purl_single_args})
    list(APPEND multi_args ${purl_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(supported_purl_variants QT MIRROR)
    if(NOT arg_PURL_VARIANT IN_LIST supported_purl_variants)
        message(FATAL_ERROR "PURL_VARIANT unknown: ${arg_PURL_VARIANT}")
    endif()

    if(arg_PURL_VARIANT STREQUAL "QT")
        set(purl_type "generic")
        set(purl_namespace "${arg_SUPPLIER}")
        set(purl_name "${arg_NAME}")
        set(purl_version "${arg_VERSION}")
    elseif(arg_PURL_VARIANT STREQUAL "MIRROR")
        set(purl_type "github")
        set(purl_namespace "qt")
        set(purl_name "${arg_REPO_NAME}")
        set(purl_version "${arg_VERSION}")
    endif()

    if(arg_PURL_TYPE)
        set(purl_type "${arg_PURL_TYPE}")
    endif()

    if(arg_PURL_NAMESPACE)
        set(purl_namespace "${arg_PURL_NAMESPACE}")
    endif()

    if(arg_PURL_NAME)
        set(purl_name "${arg_PURL_NAME}")
    endif()

    if(arg_PURL_VERSION)
        set(purl_version "${arg_PURL_VERSION}")
    endif()

    set(purl_version_option "")
    if(purl_version)
        set(purl_version_option PURL_VERSION "${purl_version}")
    endif()

    set(purl_args
        PURL_TYPE "${purl_type}"
        PURL_NAMESPACE "${purl_namespace}"
        PURL_NAME "${purl_name}"
        ${purl_version_option}
    )

    if(arg_PURL_QUALIFIERS)
        list(APPEND purl_args PURL_QUALIFIERS "${arg_PURL_QUALIFIERS}")
    endif()

    if(arg_PURL_SUBPATH)
        list(APPEND purl_args PURL_SUBPATH "${arg_PURL_SUBPATH}")
    endif()

    set(${arg_OUT_VAR} "${purl_args}" PARENT_SCOPE)
endfunction()

# Assembles an external reference purl identifier.
# PURL_TYPE and PURL_NAME are required.
# Stores the result in the OUT_VAR.
# Accepted options:
#    PURL_TYPE
#    PURL_NAME
#    PURL_NAMESPACE
#    PURL_VERSION
#    PURL_SUBPATH
#    PURL_QUALIFIERS
function(_qt_internal_sbom_assemble_purl target)
    set(opt_args "")
    set(single_args
        OUT_VAR
    )
    set(multi_args "")

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)
    list(APPEND opt_args ${purl_opt_args})
    list(APPEND single_args ${purl_single_args})
    list(APPEND multi_args ${purl_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(purl_scheme "pkg")

    if(NOT arg_PURL_TYPE)
        message(FATAL_ERROR "PURL_TYPE must be set")
    endif()

    if(NOT arg_PURL_NAME)
        message(FATAL_ERROR "PURL_NAME must be set")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    # https://github.com/package-url/purl-spec
    # Spec is 'scheme:type/namespace/name@version?qualifiers#subpath'
    set(purl "${purl_scheme}:${arg_PURL_TYPE}")

    if(arg_PURL_NAMESPACE)
        string(APPEND purl "/${arg_PURL_NAMESPACE}")
    endif()

    string(APPEND purl "/${arg_PURL_NAME}")

    if(arg_PURL_VERSION)
        string(APPEND purl "@${arg_PURL_VERSION}")
    endif()

    if(arg_PURL_QUALIFIERS)
        # TODO: Note that the qualifiers are expected to be URL encoded, which this implementation
        # is not doing at the moment.
        list(JOIN arg_PURL_QUALIFIERS "&" qualifiers)
        string(APPEND purl "?${qualifiers}")
    endif()

    if(arg_PURL_SUBPATH)
        string(APPEND purl "#${arg_PURL_SUBPATH}")
    endif()

    _qt_internal_sbom_get_purl_value_extref(VALUE "${purl}" OUT_VAR result)

    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

# Takes a PURL VALUE and returns an SBOM purl external reference in OUT_VAR.
function(_qt_internal_sbom_get_purl_value_extref)
    set(opt_args "")
    set(single_args
        OUT_VAR
        VALUE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    if(NOT arg_VALUE)
        message(FATAL_ERROR "VALUE must be set")
    endif()

    # SPDX SBOM External reference type.
    set(ext_ref_prefix "PACKAGE-MANAGER purl")
    set(external_ref "${ext_ref_prefix} ${arg_VALUE}")
    set(result "EXTREF" "${external_ref}")
    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()
