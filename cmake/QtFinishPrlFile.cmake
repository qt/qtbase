# Finish a preliminary .prl file.
#
# - Replaces occurrences of the build libdir with $$[QT_INSTALL_LIBDIR/get].
# - Strips version number suffixes from absolute paths, because qmake's lflag
#   merging does not handle them correctly.
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILE: path to the preliminary .prl file
# OUT_FILE: path to the final .prl file that's going to be installed
# QT_BUILD_LIBDIR: path to Qt's libdir when building (those paths get replaced)
# LIBRARY_SUFFIXES: list of known library extensions, e.g. .so;.a on Linux

function(strip_library_version_suffix out_var file_path)
    get_filename_component(dir "${file_path}" DIRECTORY)
    get_filename_component(basename "${file_path}" NAME_WE)
    get_filename_component(ext "${file_path}" EXT)
    foreach(libsuffix ${LIBRARY_SUFFIXES})
        if(ext MATCHES "^${libsuffix}(\\.[0-9]+)+")
            set(ext ${libsuffix})
            break()
        endif()
    endforeach()
    set(${out_var} "${dir}/${basename}${ext}" PARENT_SCOPE)
endfunction()

file(STRINGS "${IN_FILE}" lines)
set(content "")
foreach(line ${lines})
    if(line MATCHES "^QMAKE_PRL_LIBS_FOR_CMAKE = (.*)")
        unset(adjusted_libs)
        foreach(lib ${CMAKE_MATCH_1})
            if("${lib}" STREQUAL "")
                continue()
            endif()
            if(IS_ABSOLUTE "${lib}")
                strip_library_version_suffix(lib "${lib}")
                file(RELATIVE_PATH relative_lib "${QT_BUILD_LIBDIR}" "${lib}")
                if(IS_ABSOLUTE "${relative_lib}" OR (relative_lib MATCHES "^\\.\\."))
                    list(APPEND adjusted_libs "${lib}")
                else()
                    list(APPEND adjusted_libs "$$[QT_INSTALL_LIBS/get]/${relative_lib}")
                endif()
            else()
                if(NOT lib MATCHES "^-l" AND NOT lib MATCHES "^-framework")
                    string(PREPEND lib "-l")
                endif()
                list(APPEND adjusted_libs "${lib}")
            endif()
        endforeach()
        list(JOIN adjusted_libs " " adjusted_libs_for_qmake)
        string(APPEND content "QMAKE_PRL_LIBS = ${adjusted_libs_for_qmake}\n")
        string(APPEND content "QMAKE_PRL_LIBS_FOR_CMAKE = ${adjusted_libs}\n")
    else()
        string(APPEND content "${line}\n")
    endif()
endforeach()
file(WRITE "${OUT_FILE}" "${content}")
