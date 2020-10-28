# Finish a preliminary .prl file.
#
# - Replaces occurrences of the build libdir with $$[QT_INSTALL_LIBDIR].
# - Strips version number suffixes from absolute paths, because qmake's lflag
#   merging does not handle them correctly.
# - Transforms absolute library paths into link flags
#   aka from "/usr/lib/x86_64-linux-gnu/libcups.so" to "-lcups"
# - Replaces Qt absolute framework paths into a combination of -F$$[QT_INSTALL_LIBS] and
#   -framework QtFoo
# - Prepends '-l' to values that are not absolute paths, and don't start with a dash
#   aka, '-lfoo', '-framework', '-pthread'.
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILE: path to the preliminary .prl file
# OUT_FILE: path to the final .prl file that's going to be installed
# QT_LIB_DIRS: list of paths where Qt libraries are located.
#              This includes the install prefix and the current repo build dir.
#              These paths get replaced with relocatable paths or linker / framework flags.
# LIBRARY_SUFFIXES: list of known library extensions, e.g. .so;.a on Linux
# LIBRARY_PREFIXES: list of known library prefies, e.g. the "lib" in "libz" on on Linux
# LINK_LIBRARY_FLAG: flag used to link a shared library to an executable, e.g. -l on UNIX

include("${CMAKE_CURRENT_LIST_DIR}/QtGenerateLibHelpers.cmake")

file(STRINGS "${IN_FILE}" lines)
set(content "")
set(qt_framework_search_path_inserted FALSE)
foreach(line ${lines})
    if(line MATCHES "^RCC_OBJECTS = (.*)")
        set(rcc_objects ${CMAKE_MATCH_1})
    elseif(line MATCHES "^QMAKE_PRL_LIBS_FOR_CMAKE = (.*)")
        unset(adjusted_libs)
        foreach(lib ${CMAKE_MATCH_1})
            if("${lib}" STREQUAL "")
                continue()
            endif()

            # Check if the absolute path represents a Qt module located either in Qt's
            # $prefix/lib dir, or in the build dir of the repo.
            if(IS_ABSOLUTE "${lib}")
                qt_internal_path_is_relative_to_qt_lib_path(
                    "${lib}" "${QT_LIB_DIRS}" lib_is_a_qt_module relative_lib)
                if(NOT lib_is_a_qt_module)
                    # It's not a Qt module, extract the library name and prepend an -l to make
                    # it relocatable.
                    qt_transform_absolute_library_paths_to_link_flags(lib_with_link_flag "${lib}")
                    list(APPEND adjusted_libs "${lib_with_link_flag}")
                else()
                    # Is a Qt module.
                    # Transform Qt framework paths into -framework flags.
                    if(relative_lib MATCHES "^(Qt(.+))\\.framework/")
                        if(NOT qt_framework_search_path_inserted)
                            set(qt_framework_search_path_inserted TRUE)
                            list(APPEND adjusted_libs "-F$$[QT_INSTALL_LIBS]")
                        endif()
                        list(APPEND adjusted_libs "-framework" "${CMAKE_MATCH_1}")
                    else()
                        # Not a framework, transform the Qt module into relocatable relative path.
                        qt_strip_library_version_suffix(relative_lib "${relative_lib}")
                        list(APPEND adjusted_libs "$$[QT_INSTALL_LIBS]/${relative_lib}")
                    endif()
                endif()
            else()
                # Not absolute path, most likely a library name or a linker flag.
                # If linker flag (like -framework, -lfoo, -pthread, keep it as-is).
                if(NOT lib MATCHES "^-")
                    string(PREPEND lib "-l")
                endif()
                list(APPEND adjusted_libs "${lib}")
            endif()
        endforeach()
        if(rcc_objects)
            list(APPEND adjusted_libs ${rcc_objects})
        endif()
        list(JOIN adjusted_libs " " adjusted_libs_for_qmake)
        string(APPEND content "QMAKE_PRL_LIBS = ${adjusted_libs_for_qmake}\n")
        string(APPEND content "QMAKE_PRL_LIBS_FOR_CMAKE = ${adjusted_libs}\n")
    else()
        string(APPEND content "${line}\n")
    endif()
endforeach()
file(WRITE "${OUT_FILE}" "${content}")

# Read the prl meta file to find out where should the final prl file be placed,
# Copy it there, if the contents hasn't changed.
file(STRINGS "${IN_META_FILE}" lines)

foreach(line ${lines})
    if(line MATCHES "^FINAL_PRL_FILE_PATH = (.*)")
        set(final_prl_file_path "${CMAKE_MATCH_1}")
        configure_file(
            "${OUT_FILE}"
            "${final_prl_file_path}"
            COPYONLY
            )
    endif()
endforeach()
