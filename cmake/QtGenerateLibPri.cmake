# Generate a qt_lib_XXX.pri file.
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILES: path to the qt_lib_XXX.cmake files
# OUT_FILE: path to the generated qt_lib_XXX.pri file
# CONFIGS: the configurations Qt is being built with
#
# QMAKE_LIBS_XXX values are split into QMAKE_LIBS_XXX_DEBUG and QMAKE_LIBS_XXX_RELEASE if
# debug_and_release was detected. The CMake configuration "Debug" is considered for the _DEBUG
# values. The first config that is not "Debug" is treated as _RELEASE.

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
        if(value)
            qmake_list(value ${value})
            string(APPEND content "QMAKE_${infix}_${lib} = ${value}\n")
        endif()
    endforeach()
endforeach()
file(WRITE "${OUT_FILE}" "${content}")
