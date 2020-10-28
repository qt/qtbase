# Add libraries to variable ${out_libs_var} in a way that duplicates
# are added at the end. This ensures the library order needed for the
# linker.
function(qt_merge_libs out_libs_var)
    foreach(dep ${ARGN})
        list(REMOVE_ITEM ${out_libs_var} ${dep})
        list(APPEND ${out_libs_var} ${dep})
    endforeach()
    set(${out_libs_var} ${${out_libs_var}} PARENT_SCOPE)
endfunction()

# Collects the library dependencies of a target.
# This takes into account transitive usage requirements.
function(qt_collect_libs target out_var)
    qt_internal_walk_libs("${target}" "${out_var}" "qt_collect_libs_dict" "collect_libs")
    set("${out_var}" "${${out_var}}" PARENT_SCOPE)
endfunction()

# Walks a target's link libraries recursively, and performs some actions (poor man's polypmorphism)
#
# out_var is the name of the variable where the result will be assigned. The result is a list of
# libraries, mostly in generator expression form.
# dict_name is used for caching the result, and preventing the same target from being processed
# twice
# operation is a string to tell the function what to do
function(qt_internal_walk_libs target out_var dict_name operation)
    set(collected ${ARGN})
    if(target IN_LIST collected)
        return()
    endif()
    list(APPEND collected ${target})

    if(target STREQUAL "${QT_CMAKE_EXPORT_NAMESPACE}::Startup")
        # We can't (and don't need to) process Startup, because it contains $<TARGET_PROPERTY:prop>
        # genexes which get replaced with $<TARGET_PROPERTY:Startup,prop> genexes in the code below
        # and that causes 'INTERFACE_LIBRARY targets may only have whitelisted properties.' errors
        # with CMake versions equal to or lower than 3.18. These errors are super unintuitive to
        # debug because there's no mention that it's happening during a file(GENERATE) call.
        return()
    endif()

    if(NOT TARGET ${dict_name})
        add_library(${dict_name} INTERFACE IMPORTED GLOBAL)
    endif()
    get_target_property(libs ${dict_name} INTERFACE_${target})
    if(NOT libs)
        unset(libs)
        get_target_property(target_libs ${target} INTERFACE_LINK_LIBRARIES)
        if(NOT target_libs)
            unset(target_libs)
        endif()
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY")
            get_target_property(link_libs ${target} LINK_LIBRARIES)
            if(link_libs)
                list(APPEND target_libs ${link_libs})
            endif()
        endif()
        foreach(lib ${target_libs})
            # Cannot use $<TARGET_POLICY:...> in add_custom_command.
            # Check the policy now, and replace the generator expression with the value.
            while(lib MATCHES "\\$<TARGET_POLICY:([^>]+)>")
                cmake_policy(GET ${CMAKE_MATCH_1} value)
                if(value STREQUAL "NEW")
                    set(value "TRUE")
                else()
                    set(value "FALSE")
                endif()
                string(REPLACE "${CMAKE_MATCH_0}" "${value}" lib "${lib}")
            endwhile()

            # Fix up $<TARGET_PROPERTY:FOO> expressions that refer to the "current" target.
            # Those cannot be used with add_custom_command.
            while(lib MATCHES "\\$<TARGET_PROPERTY:([^,>]+)>")
                string(REPLACE "${CMAKE_MATCH_0}" "$<TARGET_PROPERTY:${target},${CMAKE_MATCH_1}>"
                    lib "${lib}")
            endwhile()

            # Skip static plugins.
            set(_is_plugin_marker_genex "\\$<BOOL:QT_IS_PLUGIN_GENEX>")
            if(lib MATCHES "${_is_plugin_marker_genex}")
                continue()
            endif()

            # Strip any directory scope tokens.
            qt_internal_strip_target_directory_scope_token("${lib}" lib)

            if(lib MATCHES "^\\$<TARGET_OBJECTS:")
                # Skip object files.
                continue()
            elseif(lib MATCHES "^\\$<LINK_ONLY:(.*)>$")
                set(lib_target ${CMAKE_MATCH_1})
            else()
                set(lib_target ${lib})
            endif()

            # Skip CMAKE_DIRECTORY_ID_SEP. If a target_link_libraries is applied to a target
            # that was defined in a different scope, CMake appends and prepends a special directory
            # id separator. Filter those out.
            if(lib_target MATCHES "^::@")
                continue()
            elseif(TARGET ${lib_target})
                if ("${lib_target}" MATCHES "^Qt::(.*)")
                    # If both, Qt::Foo and Foo targets exist, prefer the target name without
                    # namespace. Which one is preferred doesn't really matter. This code exists to
                    # avoid ending up with both, Qt::Foo and Foo in our dependencies.
                    set(namespaceless_lib_target "${CMAKE_MATCH_1}")
                    if(TARGET namespaceless_lib_target)
                        set(lib_target ${namespaceless_lib_target})
                    endif()
                endif()
                get_target_property(lib_target_type ${lib_target} TYPE)
                if(lib_target_type STREQUAL "INTERFACE_LIBRARY")
                    qt_internal_walk_libs(
                        ${lib_target} lib_libs "${dict_name}" "${operation}" ${collected})
                    if(lib_libs)
                        qt_merge_libs(libs ${lib_libs})
                        set(is_module 0)
                    endif()
                else()
                    qt_merge_libs(libs "$<TARGET_FILE:${lib_target}>")
                    qt_internal_walk_libs(
                        ${lib_target} lib_libs "${dict_name}" "${operation}" ${collected})
                    if(lib_libs)
                        qt_merge_libs(libs ${lib_libs})
                    endif()
                endif()
                if(operation STREQUAL "promote_global")
                    set(lib_target_unaliased "${lib_target}")
                    get_target_property(aliased_target ${lib_target} ALIASED_TARGET)
                    if(aliased_target)
                        set(lib_target_unaliased ${aliased_target})
                    endif()

                    get_property(is_imported TARGET ${lib_target_unaliased} PROPERTY IMPORTED)
                    get_property(is_global TARGET ${lib_target_unaliased} PROPERTY IMPORTED_GLOBAL)

                    # Allow opting out of promotion. This is useful in certain corner cases
                    # like with WrapLibClang and Threads in qttools.
                    qt_internal_should_not_promote_package_target_to_global(
                        "${lib_target_unaliased}" should_not_promote)
                    if(NOT is_global AND is_imported AND NOT should_not_promote)
                        set_property(TARGET ${lib_target_unaliased} PROPERTY IMPORTED_GLOBAL TRUE)
                    endif()
                endif()
            else()
                set(final_lib_name_to_merge "${lib_target}")
                if(lib_target MATCHES "/([^/]+).framework$")
                    set(final_lib_name_to_merge "-framework ${CMAKE_MATCH_1}")
                endif()
                qt_merge_libs(libs "${final_lib_name_to_merge}")
            endif()
        endforeach()
        set_target_properties(${dict_name} PROPERTIES INTERFACE_${target} "${libs}")
    endif()
    set(${out_var} ${libs} PARENT_SCOPE)
endfunction()

# Generate a qmake .prl file for the given target.
# The install_dir argument is a relative path, for example "lib".
function(qt_generate_prl_file target install_dir)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    get_target_property(rcc_objects ${target} QT_RCC_OBJECTS)
    if(rcc_objects)
        if(QT_WILL_INSTALL)
            list(TRANSFORM rcc_objects PREPEND "$$[QT_INSTALL_LIBS]/")
        endif()
    else()
        unset(rcc_objects)
    endif()

    unset(prl_config)
    set(is_static FALSE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        list(APPEND prl_config static)
        set(is_static TRUE)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        list(APPEND prl_config shared)
    endif()
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_fw ${target} FRAMEWORK)
        if(is_fw)
            list(APPEND prl_config lib_bundle)
        endif()
    endif()
    list(JOIN prl_config " " prl_config)

    # Generate a preliminary .prl file that contains absolute paths to all libraries
    if(MINGW)
        # For MinGW, qmake doesn't have a lib prefix in prl files.
        set(prefix_for_final_prl_name "")
    else()
        set(prefix_for_final_prl_name "$<TARGET_FILE_PREFIX:${target}>")
    endif()

    # For frameworks, the prl file should be placed under the Resources subdir.
    get_target_property(is_framework ${target} FRAMEWORK)
    if(is_framework)
        get_target_property(fw_version ${target} FRAMEWORK_VERSION)
        string(APPEND prefix_for_final_prl_name "Versions/${fw_version}/Resources/")
    endif()

    # What follows is a complicated setup for generating configuration-specific
    # prl files. It has to be this way, because add_custom_command doesn't support
    # generator expressions in OUTPUT or DEPENDS.
    # To circumvent that, we create well known file names with file(GENERATE)
    # with configuration specific content, which are then fed to add_custom_command
    # that uses these genex-less file names. The actual command will extract the info
    # from the configuration-specific files, and create a properly named final prl file.

    # The file is named according to a pattern, that is then used in the
    # add_custom_command.
    set(prl_step1_name_prefix "preliminary_prl_for_${target}_step1_")
    set(prl_step1_name_suffix ".prl" )
    qt_path_join(prl_step1_path
                 "${CMAKE_CURRENT_BINARY_DIR}"
                 "${prl_step1_name_prefix}$<CONFIG>${prl_step1_name_suffix}")

    # Same, except instead of containing the prl contents, it will contain the final prl file
    # name computed via a generator expression.
    set(prl_meta_info_name_prefix "preliminary_prl_meta_info_for_${target}_")
    set(prl_meta_info_name_suffix ".txt")
    qt_path_join(prl_meta_info_path
                 "${CMAKE_CURRENT_BINARY_DIR}"
                 "${prl_meta_info_name_prefix}$<CONFIG>${prl_meta_info_name_suffix}")

    # The final prl file name that will be embedded in the file above.
    set(final_prl_file_name "${prefix_for_final_prl_name}$<TARGET_FILE_BASE_NAME:${target}>.prl")
    qt_path_join(final_prl_file_path "${QT_BUILD_DIR}/${install_dir}" "${final_prl_file_name}")

    # Generate the prl content and its final file name into configuration specific files
    # whose names we know, and can be used in add_custom_command.
    set(prl_step1_content
        "RCC_OBJECTS = ${rcc_objects}
QMAKE_PRL_BUILD_DIR = ${CMAKE_CURRENT_BINARY_DIR}
QMAKE_PRL_TARGET = $<TARGET_FILE_NAME:${target}>
QMAKE_PRL_CONFIG = ${prl_config}
QMAKE_PRL_VERSION = ${PROJECT_VERSION}
")
    if(NOT is_static AND WIN32)
        # Do nothing. Prl files for shared libraries on Windows shouldn't have the libs listed,
        # as per qt_build_config.prf and the conditional CONFIG+=explicitlib assignment.
    else()
        set(prl_libs "")
        qt_collect_libs(${target} prl_libs)
        string(APPEND prl_step1_content "QMAKE_PRL_LIBS_FOR_CMAKE = ${prl_libs}\n")
    endif()

    file(GENERATE
        OUTPUT "${prl_step1_path}"
        CONTENT "${prl_step1_content}")
    file(GENERATE
         OUTPUT "${prl_meta_info_path}"
         CONTENT
         "FINAL_PRL_FILE_PATH = ${final_prl_file_path}")

    set(library_prefixes ${CMAKE_SHARED_LIBRARY_PREFIX} ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(library_suffixes
        ${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}
        ${CMAKE_STATIC_LIBRARY_SUFFIX})

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()

    set(qt_lib_dirs "${QT_BUILD_DIR}/${INSTALL_LIBDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_lib_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()

    foreach(config ${configs})
        # Output file for dependency tracking, and which will contain the final content.
        qt_path_join(prl_step2_path
                     "${CMAKE_CURRENT_BINARY_DIR}" "preliminary_prl_for_${target}_step2_${config}.prl")

        # Input dependency names that are constructed for each config manually
        # (no genexes allowed).
        qt_path_join(prl_step1_path
                     "${CMAKE_CURRENT_BINARY_DIR}"
                     "${prl_step1_name_prefix}${config}${prl_step1_name_suffix}")
        qt_path_join(prl_meta_info_path
                     "${CMAKE_CURRENT_BINARY_DIR}"
                     "${prl_meta_info_name_prefix}${config}${prl_meta_info_name_suffix}")
        add_custom_command(
            OUTPUT  "${prl_step2_path}"
            DEPENDS "${prl_step1_path}"
                    "${prl_meta_info_path}"
                    "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
                    "${QT_CMAKE_DIR}/QtGenerateLibHelpers.cmake"
            COMMAND ${CMAKE_COMMAND}
                    "-DIN_FILE=${prl_step1_path}"
                    "-DIN_META_FILE=${prl_meta_info_path}"
                    "-DOUT_FILE=${prl_step2_path}"
                    "-DLIBRARY_PREFIXES=${library_prefixes}"
                    "-DLIBRARY_SUFFIXES=${library_suffixes}"
                    "-DLINK_LIBRARY_FLAG=${CMAKE_LINK_LIBRARY_FLAG}"
                    "-DQT_LIB_DIRS=${qt_lib_dirs}"
                    -P "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
            VERBATIM
            COMMENT "Generating prl file for target ${target}"
            )

        # Tell the target to depend on the preliminary prl file, to ensure the custom command
        # is executed. As a side-effect, this will also create the final prl file that
        # is named appropriately. It should not be specified as a BYPRODUCT.
        # This allows proper per-file dependency tracking, without having to resort on a POST_BUILD
        # step, which means that relinking would happen as well as transitive rebuilding of any
        # dependees.
        # This is inspired by https://gitlab.kitware.com/cmake/cmake/-/issues/20842
        target_sources(${target} PRIVATE "${prl_step2_path}")
    endforeach()

    # Installation of the .prl file happens globally elsewhere,
    # because we have no clue here what the actual file name is.
    # What we know however, is the directory where the prl file is created.
    # Save that for later, to install all prl files from that directory.
    get_property(prl_install_dirs GLOBAL PROPERTY QT_PRL_INSTALL_DIRS)
    if(NOT install_dir IN_LIST prl_install_dirs)
        set_property(GLOBAL APPEND PROPERTY QT_PRL_INSTALL_DIRS "${install_dir}")
    endif()
endfunction()
