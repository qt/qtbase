# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collects the library dependencies of a target.
# As well as rcc object file dependencies.
# This takes into account transitive usage requirements.
function(qt_collect_libs target libs_out_var rcc_objects_out_var)
    __qt_internal_walk_libs("${target}" "${libs_out_var}"
                            "${rcc_objects_out_var}" "qt_collect_libs_dict" "collect_libs")
    set("${libs_out_var}" "${${libs_out_var}}" PARENT_SCOPE)

    set(${rcc_objects_out_var} "${${rcc_objects_out_var}}" PARENT_SCOPE)

endfunction()

# Generate a qmake .prl file for the given target.
# The install_dir argument is a relative path, for example "lib".
function(qt_generate_prl_file target install_dir)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    unset(prl_config)
    set(is_static FALSE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        list(APPEND prl_config static)
        set(is_static TRUE)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        list(APPEND prl_config shared)
    endif()
    get_target_property(is_fw ${target} FRAMEWORK)
    if(is_fw)
        list(APPEND prl_config lib_bundle)
    endif()
    list(JOIN prl_config " " prl_config)

    set(rcc_objects "")
    set(prl_step1_content_libs "")
    if(NOT is_static AND WIN32)
        # Do nothing. Prl files for shared libraries on Windows shouldn't have the libs listed,
        # as per qt_build_config.prf and the conditional CONFIG+=explicitlib assignment.
    else()
        set(prl_libs "")
        qt_collect_libs(${target} prl_libs prl_rcc_objects)
        if(prl_libs)
            set(prl_step1_content_libs "QMAKE_PRL_LIBS_FOR_CMAKE = ${prl_libs}\n")
        endif()
        if(prl_rcc_objects)
            list(APPEND rcc_objects ${prl_rcc_objects})
        endif()
    endif()

    if(rcc_objects AND QT_WILL_INSTALL)
        list(TRANSFORM rcc_objects PREPEND "$$[QT_INSTALL_PREFIX]/")
    endif()

    # Generate a preliminary .prl file that contains absolute paths to all libraries
    if(MINGW)
        # For MinGW, qmake doesn't have a lib prefix in prl files.
        set(prefix_for_final_prl_name "")
    else()
        set(prefix_for_final_prl_name "$<TARGET_FILE_PREFIX:${target}>")
    endif()

    # For macOS frameworks, the prl file should be placed under the Resources subdir.
    # For iOS, visionOS, watchOS, tvOS, there is no Resources subdir, and the contents needs to
    # be placed directly in the framework root, as described at
    # https://developer.apple.com/documentation/bundleresources/placing-content-in-a-bundle?language=objc
    get_target_property(is_framework ${target} FRAMEWORK)
    if(APPLE AND (NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
        set(is_macos TRUE)
    else()
        set(is_macos FALSE)
    endif()
    if(is_framework AND is_macos)
        get_target_property(fw_version ${target} FRAMEWORK_VERSION)
        string(APPEND prefix_for_final_prl_name "Versions/${fw_version}/Resources/")
    endif()

    # Generate configuration-specific prl files in two steps:
    # - step1: a file(GENERATE) writes the preliminary prl content with absolute paths.
    # - step2: an add_custom_command runs QtFinishPrlFile.cmake to transform absolute paths into
    #          qmake link flags / relocatable references and write the final prl file.
    set(prl_step1_path
        "${CMAKE_CURRENT_BINARY_DIR}/preliminary_prl_for_${target}_step1_$<CONFIG>.prl")

    # The final prl file path is passed to the custom command verbatim; its
    # $<TARGET_FILE_BASE_NAME> genex is evaluated at build time per config.
    set(final_prl_file_name "${prefix_for_final_prl_name}$<TARGET_FILE_BASE_NAME:${target}>")
    if(ANDROID)
        string(APPEND final_prl_file_name "_${CMAKE_ANDROID_ARCH_ABI}")
    endif()
    string(APPEND final_prl_file_name ".prl")
    qt_path_join(final_prl_file_path "${QT_BUILD_DIR}/${install_dir}" "${final_prl_file_name}")

    set(prl_step1_content
        "RCC_OBJECTS = ${rcc_objects}
QMAKE_PRL_TARGET = $<TARGET_LINKER_FILE_NAME:${target}>
QMAKE_PRL_TARGET_PATH_FOR_CMAKE = $<TARGET_LINKER_FILE:${target}>
QMAKE_PRL_CONFIG = ${prl_config}
QMAKE_PRL_VERSION = ${PROJECT_VERSION}
${prl_step1_content_libs}
")

    file(GENERATE
        OUTPUT "${prl_step1_path}"
        CONTENT "${prl_step1_content}")

    set(library_prefixes ${CMAKE_SHARED_LIBRARY_PREFIX} ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(library_suffixes
        ${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}
        ${CMAKE_STATIC_LIBRARY_SUFFIX})

    set(qt_lib_dirs "${QT_BUILD_DIR}/${INSTALL_LIBDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_lib_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()

    set(qt_plugin_dirs "${QT_BUILD_DIR}/${INSTALL_PLUGINSDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_plugin_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_PLUGINSDIR}")
    endif()

    set(qt_qml_dirs "${QT_BUILD_DIR}/${INSTALL_QMLDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_qml_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_QMLDIR}")
    endif()

    if(MSVC)
        set(link_library_flag "-l")
        file(TO_CMAKE_PATH "$ENV{LIB};${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}" implicit_link_directories)
    else()
        set(link_library_flag ${CMAKE_LINK_LIBRARY_FLAG})
        set(implicit_link_directories ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
    endif()

    set(prl_step2_path
        "${CMAKE_CURRENT_BINARY_DIR}/preliminary_prl_for_${target}_step2_$<CONFIG>.prl")

    add_custom_command(
        OUTPUT  "${prl_step2_path}"
        DEPENDS "${prl_step1_path}"
                "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
                "${QT_CMAKE_DIR}/QtGenerateLibHelpers.cmake"
        COMMAND ${CMAKE_COMMAND}
                "-DIN_FILE=${prl_step1_path}"
                "-DOUT_FILE=${prl_step2_path}"
                "-DFINAL_PRL_FILE_PATH=${final_prl_file_path}"
                "-DLIBRARY_PREFIXES=${library_prefixes}"
                "-DLIBRARY_SUFFIXES=${library_suffixes}"
                "-DLINK_LIBRARY_FLAG=${link_library_flag}"
                "-DQT_LIB_DIRS=${qt_lib_dirs}"
                "-DQT_PLUGIN_DIRS=${qt_plugin_dirs}"
                "-DQT_QML_DIRS=${qt_qml_dirs}"
                "-DIMPLICIT_LINK_DIRECTORIES=${implicit_link_directories}"
                -P "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
        VERBATIM
        COMMENT "Generating prl file for target ${target}"
        )

    # Attach the per-config preliminary prl file to the library target so that the custom command
    # runs before linking. This is inspired by
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20842
    # add_custom_command(OUTPUT) does not register a path containing a generator expression as a
    # GENERATED source, so at generation time the per-config source resolution for target_sources
    # cannot match it and fails with "Cannot find source file" (CMake bug
    # https://gitlab.kitware.com/cmake/cmake/-/issues/24678). Pre-expand the path per config.
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(prl_configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(prl_configs ${CMAKE_BUILD_TYPE})
    endif()
    foreach(prl_config ${prl_configs})
        string(REPLACE "$<CONFIG>" "${prl_config}" prl_step2_path_for_config "${prl_step2_path}")
        target_sources(${target} PRIVATE "${prl_step2_path_for_config}")
    endforeach()

    # Install the final .prl file that's generated by the QtFinishPrlFile.cmake script.
    # For Apple frameworks, the .prl file is already placed inside the framework.
    if(NOT is_framework)
        qt_install(FILES "${final_prl_file_path}" DESTINATION "${install_dir}")
    endif()
endfunction()
