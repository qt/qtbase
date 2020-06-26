# Generate a qt_lib_XXX.pri file.
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILES: path to the qt_lib_XXX.cmake files
# OUT_FILE: path to the generated qt_lib_XXX.pri file
# CONFIGS: the configurations Qt is being built with
# LIBRARY_SUFFIXES: list of known library extensions, e.g. .so;.a on Linux
# LIBRARY_PREFIXES: list of known library prefies, e.g. the "lib" in "libz" on on Linux
# LINK_LIBRARY_FLAG: flag used to link a shared library to an executable, e.g. -l on UNIX
#
# QMAKE_LIBS_XXX values are split into QMAKE_LIBS_XXX_DEBUG and QMAKE_LIBS_XXX_RELEASE if
# debug_and_release was detected. The CMake configuration "Debug" is considered for the _DEBUG
# values. The first config that is not "Debug" is treated as _RELEASE.
#
# The library values are transformed from an absolute path into link flags
# aka from "/usr/lib/x86_64-linux-gnu/libcups.so" to "-lcups".

cmake_policy(SET CMP0057 NEW)

# Create a qmake-style list from the passed arguments and store it in ${out_var}.
function(qmake_list out_var)
    set(result "")

    # Surround values that contain spaces with double quotes.
    foreach(v ${ARGN})
        if(v MATCHES " ")
            set(v "\"${v}\"")
        endif()
        list(APPEND result ${v})
    endforeach()

    list(JOIN result " " result)
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

# Given "/usr/lib/x86_64-linux-gnu/libcups.so"
# Returns "cups" or an empty string if the file is not an absolute library path.
# Aka it strips the "lib" prefix, the .so extension and the base path.
function(qt_get_library_name_without_prefix_and_suffix out_var file_path)
    set(out_value "")
    if(IS_ABSOLUTE "${file_path}")
        get_filename_component(basename "${file_path}" NAME_WE)
        get_filename_component(ext "${file_path}" EXT)
        foreach(libsuffix ${LIBRARY_SUFFIXES})
            # Handle weird prefix extensions like in the case of
            # "/usr/lib/x86_64-linux-gnu/libglib-2.0.so"
            # it's ".0.so".
            if(ext MATCHES "^(\\.[0-9]+)*${libsuffix}(\\.[0-9]+)*")
                set(is_linkable_library TRUE)
                set(weird_numbered_extension "${CMAKE_MATCH_1}")
                break()
            endif()
        endforeach()
        if(is_linkable_library)
            set(out_value "${basename}")
            if(LIBRARY_PREFIXES)
                foreach(libprefix ${LIBRARY_PREFIXES})
                    # Strip any library prefix like "lib" for a library that we will use with a link
                    # flag.
                    if(libprefix AND out_value MATCHES "^${libprefix}(.+)")
                        set(out_value "${CMAKE_MATCH_1}")
                        break()
                    endif()
                endforeach()
            endif()
            if(weird_numbered_extension)
                set(out_value "${out_value}${weird_numbered_extension}")
            endif()
        endif()
    endif()

    # Reverse the dependency order to be in sync with what qmake generated .pri files
    # have.
    list(REVERSE out_list)

    set(${out_var} "${out_value}" PARENT_SCOPE)
endfunction()

# Given "/usr/lib/x86_64-linux-gnu/libcups.so"
# Returns "-lcups" or an empty string if the file is not an absolute library path.
function(qt_get_library_with_link_flag out_var file_path)
    qt_get_library_name_without_prefix_and_suffix(lib_name "${file_path}")

    set(out_value "")
    if(lib_name)
        set(out_value "${lib_name}")
        if(LINK_LIBRARY_FLAG)
            string(PREPEND out_value "${LINK_LIBRARY_FLAG}")
        endif()
    endif()

    set(${out_var} "${out_value}" PARENT_SCOPE)
endfunction()

# Given a list of potential library paths, returns a transformed list where absolute library paths
# are replaced with library link flags.
function(qt_transform_absolute_library_paths_to_link_flags out_var library_path_list)
    set(out_list "")
    foreach(library_path ${library_path_list})
        qt_get_library_with_link_flag(lib_name_with_link_flag "${library_path}")
        if(lib_name_with_link_flag)
            list(APPEND out_list "${lib_name_with_link_flag}")
        else()
            list(APPEND out_list "${library_path}")
        endif()
    endforeach()
    set(${out_var} "${out_list}" PARENT_SCOPE)
endfunction()

list(POP_FRONT IN_FILES in_pri_file)
file(READ ${in_pri_file} content)
string(APPEND content "\n")
foreach(in_file ${IN_FILES})
    include(${in_file})
endforeach()
list(REMOVE_DUPLICATES known_libs)

set(is_debug_and_release FALSE)
if("Debug" IN_LIST CONFIGS AND ("Release" IN_LIST CONFIGS OR "RelWithDebInfo" IN_LIST CONFIGS))
    set(is_debug_and_release TRUE)
    set(release_configs ${CONFIGS})
    list(REMOVE_ITEM release_configs "Debug")
    list(GET release_configs 0 release_cfg)
    string(TOUPPER "${release_cfg}" release_cfg)
endif()

foreach(lib ${known_libs})
    set(configuration_independent_infixes LIBDIR INCDIR DEFINES)

    if(is_debug_and_release)
        set(value_debug ${QMAKE_LIBS_${lib}_DEBUG})
        set(value_release ${QMAKE_LIBS_${lib}_${release_cfg}})
        qt_transform_absolute_library_paths_to_link_flags(value_debug "${value_debug}")
        qt_transform_absolute_library_paths_to_link_flags(value_release "${value_release}")

        if(value_debug STREQUAL value_release)
            if(value_debug)
                qmake_list(value_debug ${value_debug})
                string(APPEND content "QMAKE_LIBS_${lib} = ${value_debug}\n")
            endif()
        else()
            if(value_debug)
                qmake_list(value_debug ${value_debug})
                string(APPEND content "QMAKE_LIBS_${lib}_DEBUG = ${value_debug}\n")
            endif()
            if(value_release)
                qmake_list(value_release ${value_release})
                string(APPEND content "QMAKE_LIBS_${lib}_RELEASE = ${value_release}\n")
            endif()
        endif()
    else()
        list(APPEND configuration_independent_infixes LIBS)
    endif()

    # The remaining values are considered equal for all configurations.
    # Pick the first configuration and use its values.
    list(GET CONFIGS 0 cfg)
    string(TOUPPER ${cfg} cfg)
    foreach(infix ${configuration_independent_infixes})
        set(value ${QMAKE_${infix}_${lib}_${cfg}})
        if(infix STREQUAL "LIBS")
            qt_transform_absolute_library_paths_to_link_flags(value "${value}")
        endif()
        if(value)
            qmake_list(value ${value})
            string(APPEND content "QMAKE_${infix}_${lib} = ${value}\n")
        endif()
    endforeach()
endforeach()
file(WRITE "${OUT_FILE}" "${content}")
