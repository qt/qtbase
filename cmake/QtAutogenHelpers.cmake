# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Initial autogen setup for a target to specify certain CMake properties which are common
# to all autogen tools. Also enable AUTOMOC by default.
function(qt_autogen_tools_initial_setup target)
    set_property(TARGET "${target}" PROPERTY INTERFACE_QT_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
    set_property(TARGET "${target}" APPEND PROPERTY COMPATIBLE_INTERFACE_STRING QT_MAJOR_VERSION)

    set_directory_properties(PROPERTIES
        QT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
        QT_VERSION_MINOR ${PROJECT_VERSION_MINOR}
        QT_VERSION_PATCH ${PROJECT_VERSION_PATCH}
    )

    qt_enable_autogen_tool(${target} "moc" ON)
endfunction()

# Enables or disables an autogen tool like moc, uic or rcc on ${target}.
function(qt_enable_autogen_tool target tool enable)
    string(TOUPPER "${tool}" captitalAutogenTool)

    get_target_property(tool_enabled ${target} AUTO${captitalAutogenTool})
    get_target_property(autogen_target_depends ${target} AUTOGEN_TARGET_DEPENDS)

    if(NOT autogen_target_depends)
        set(autogen_target_depends "")
    endif()
    set(tool_executable "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${tool}>")
    set(tool_target_name ${QT_CMAKE_EXPORT_NAMESPACE}::${tool})

    if(enable)
        list(APPEND autogen_target_depends ${tool_target_name})
    else()
        list(REMOVE_ITEM autogen_target_depends ${tool_target_name})
    endif()

    # f66c1db16c050c9d685a44a38ad7c5cf9f6fcc96 in qtbase introduced a new macro
    # that the moc scanner has to look for. Inform the CMake moc scanner about it.
    if(tool STREQUAL "moc" AND enable)
        set_target_properties("${target}" PROPERTIES
            AUTOMOC_MACRO_NAMES "${CMAKE_AUTOMOC_MACRO_NAMES};Q_ENUM_NS;Q_GADGET_EXPORT")
        if (TARGET Qt::Platform)
            get_target_property(_abi_tag Qt::Platform qt_libcpp_abi_tag)
            if (_abi_tag)
               set_property(TARGET "${target}" APPEND PROPERTY
                  AUTOMOC_MOC_OPTIONS --libcpp-abi-version "${_abi_tag}"
               )
            endif()
        endif()
    endif()

    set_target_properties("${target}"
                          PROPERTIES
                          AUTO${captitalAutogenTool} "${enable}"
                          AUTO${captitalAutogenTool}_EXECUTABLE "${tool_executable}"
                          AUTOGEN_TARGET_DEPENDS "${autogen_target_depends}"
                          )
endfunction()

# This function adds or removes additional AUTOGEN tools to a target: AUTOMOC/UIC/RCC
function(qt_autogen_tools target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "${__default_private_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_ENABLE_AUTOGEN_TOOLS)
        foreach(tool ${arg_ENABLE_AUTOGEN_TOOLS})
            qt_enable_autogen_tool(${target} ${tool} ON)
        endforeach()
    endif()

  if(arg_DISABLE_AUTOGEN_TOOLS)
      foreach(tool ${arg_DISABLE_AUTOGEN_TOOLS})
          qt_enable_autogen_tool(${target} ${tool} OFF)
      endforeach()
  endif()
endfunction()

# Complete manual moc invocation with full control.
# Use AUTOMOC whenever possible.
# Multi-value Arguments:
#     INCLUDE_DIRECTORIES
#         Specifies a list of include directories used by 'moc'.
#     INCLUDE_DIRECTORY_TARGETS
#         Specifies a list of targets to extract the INTERFACE_INCLUDE_DIRECTORIES
#         property and use it as the 'moc' include directories.(Deprecated use TARGETS instead)
#     DEFINITIONS
#         List of the definitions that should be added to the moc command line arguments.
#         Supports the syntax both with and without the prepending '-D'.
#     TARGETS
#         The list of targets that will be used to collect the INTERFACE_INCLUDE_DIRECTORIES,
#         INCLUDE_DIRECTORIES, and COMPILE_DEFINITIONS properties.
function(qt_manual_moc result)
    cmake_parse_arguments(arg
                          ""
                          "OUTPUT_MOC_JSON_FILES"
                          "FLAGS;INCLUDE_DIRECTORIES;INCLUDE_DIRECTORY_TARGETS;DEFINITIONS;TARGETS"
                          ${ARGN})
    set(moc_files)
    set(metatypes_json_list)
    foreach(infile ${arg_UNPARSED_ARGUMENTS})
        qt_make_output_file("${infile}" "moc_" ".cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" outfile)
        list(APPEND moc_files "${outfile}")

        set(moc_parameters_file "${outfile}_parameters$<$<BOOL:$<CONFIG>>:_$<CONFIG>>")
        set(moc_parameters ${arg_FLAGS} -o "${outfile}" "${infile}")

        foreach(dir IN ITEMS ${arg_INCLUDE_DIRECTORIES})
            list(APPEND moc_parameters
                "-I\n${dir}")
        endforeach()

        foreach(dep IN LISTS arg_INCLUDE_DIRECTORY_TARGETS arg_TARGETS)
            set(include_expr "$<TARGET_PROPERTY:${dep},INTERFACE_INCLUDE_DIRECTORIES>")
            list(APPEND moc_parameters
                "$<$<BOOL:${include_expr}>:-I\n$<JOIN:${include_expr},\n-I\n>>")

            if(APPLE AND TARGET ${dep})
                get_target_property(is_versionless ${dep} _qt_is_versionless_target)
                if(is_versionless)
                    string(REGEX REPLACE "^Qt::(.*)" "\\1" dep "${dep}")
                    set(dep "${QT_CMAKE_EXPORT_NAMESPACE}::${dep}")
                endif()

                get_target_property(alias_dep ${dep} ALIASED_TARGET)
                if(alias_dep)
                    set(dep ${alias_dep})
                endif()

                get_target_property(loc ${dep} IMPORTED_LOCATION)
                string(REGEX REPLACE "(.*)/Qt[^/]+\\.framework.*" "\\1" loc "${loc}")

                if(loc)
                    list(APPEND moc_parameters "\n-F\n${loc}\n")
                endif()
            endif()
        endforeach()

        foreach(dep IN LISTS arg_TARGETS)
            set(include_property_expr
                "$<TARGET_GENEX_EVAL:${dep},$<TARGET_PROPERTY:${dep},INCLUDE_DIRECTORIES>>")
            list(APPEND moc_parameters
                "$<$<BOOL:${include_property_expr}>:-I\n$<JOIN:${include_property_expr},\n-I\n>>")

            set(defines_property_expr
                "$<TARGET_GENEX_EVAL:${dep},$<TARGET_PROPERTY:${dep},COMPILE_DEFINITIONS>>")
            set(defines_with_d "$<FILTER:${defines_property_expr},INCLUDE,^-D>")
            set(defines_without_d "$<FILTER:${defines_property_expr},EXCLUDE,^-D>")
            list(APPEND moc_parameters
                "$<$<BOOL:${defines_with_d}>:$<JOIN:${defines_with_d},\n>>")
            list(APPEND moc_parameters
                "$<$<BOOL:${defines_without_d}>:-D\n$<JOIN:${defines_without_d},\n-D\n>>")
        endforeach()

        foreach(def IN LISTS arg_DEFINITIONS)
            if(NOT def MATCHES "^-D")
                list(APPEND moc_parameters "-D\n${def}")
            else()
                list(APPEND moc_parameters "${def}")
            endif()
        endforeach()

        set(metatypes_byproducts)
        if (arg_OUTPUT_MOC_JSON_FILES)
            set(moc_json_file "${outfile}.json")
            list(APPEND moc_parameters --output-json)
            list(APPEND metatypes_json_list "${outfile}.json")
            set(metatypes_byproducts "${outfile}.json")
        endif()

        if (TARGET Qt::Platform)
           get_target_property(_abi_tag Qt::Platform qt_libcpp_abi_tag)
           if (_abi_tag)
              list(APPEND moc_parameters --libcpp-abi-version "${_abi_tag}")
           endif()
        endif()

        string (REPLACE ";" "\n" moc_parameters "${moc_parameters}")

        file(GENERATE OUTPUT "${moc_parameters_file}" CONTENT "${moc_parameters}\n")

        add_custom_command(OUTPUT "${outfile}" ${metatypes_byproducts}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc "@${moc_parameters_file}"
                           DEPENDS "${infile}" ${moc_depends} ${QT_CMAKE_EXPORT_NAMESPACE}::moc
                           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
    endforeach()
    set("${result}" ${moc_files} PARENT_SCOPE)

    # Register generated json files
    if (arg_OUTPUT_MOC_JSON_FILES)
        set(${arg_OUTPUT_MOC_JSON_FILES} "${metatypes_json_list}" PARENT_SCOPE)
    endif()
endfunction()

# From Qt6CoreMacros
# Function used to create the names of output files preserving relative dirs
function(qt_make_output_file infile prefix suffix source_dir binary_dir result)
    get_filename_component(outfilename "${infile}" NAME_WE)

    set(base_dir "${source_dir}")
    string(FIND "${infile}" "${binary_dir}/" in_binary)
    if (in_binary EQUAL 0)
        set(base_dir "${binary_dir}")
    endif()

    get_filename_component(abs_infile "${infile}" ABSOLUTE BASE_DIR "${base_dir}")
    file(RELATIVE_PATH rel_infile "${base_dir}" "${abs_infile}")
    string(REPLACE "../" "__/" mapped_infile "${rel_infile}")

    get_filename_component(abs_mapped_infile "${mapped_infile}" ABSOLUTE BASE_DIR "${binary_dir}")
    get_filename_component(outpath "${abs_mapped_infile}" PATH)

    file(MAKE_DIRECTORY "${outpath}")
    set("${result}" "${outpath}/${prefix}${outfilename}${suffix}" PARENT_SCOPE)
endfunction()

# Work around AUTOGEN issue when a library is added as a dependency more than once, and the autogen
# library dependency results in being discarded. To mitigate that, add all autogen dependencies
# manually, based on the passed in dependencies.
# CMake 4.0+ has a fix, so we don't need the extra logic.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/26700
function(qt_internal_work_around_autogen_discarded_dependencies target)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0
            OR QT_NO_AUTOGEN_DISCARDED_DEPENDENCIES_WORKAROUND)
        return()
    endif()

    set(libraries ${ARGN})
    set(final_libraries "")

    foreach(lib IN LISTS libraries)
        # Skip non-target dependencies.
        if(NOT TARGET "${lib}")
            continue()
        endif()

        # Resolve alias targets, because AUTOGEN_TARGET_DEPENDS doesn't seem to handle them.
        get_target_property(aliased_target "${lib}" ALIASED_TARGET)
        if(aliased_target)
            set(lib "${aliased_target}")
        endif()

        # Skip imported targets, they don't have sync_headers targets.
        get_target_property(imported "${lib}" IMPORTED)
        if(imported)
            continue()
        endif()

        # Resolve Qt private modules to their public counterparts.
        get_target_property(is_private_module "${lib}" _qt_is_private_module)
        get_target_property(public_module_target "${lib}" _qt_public_module_target_name)

        if(is_private_module AND public_module_target)
            set(lib "${public_module_target}")
        endif()

        # Another TARGET check, just in case.
        if(TARGET "${lib}")
            list(APPEND final_libraries "${lib}")
        endif()
    endforeach()
    if(final_libraries)
        set_property(TARGET ${target} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS "${final_libraries}")
    endif()
endfunction()
