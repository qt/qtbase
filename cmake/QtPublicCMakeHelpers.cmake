# copy_if_different works incorrect in Windows if file size if bigger than 2GB.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/23052 and QTBUG-99491 for details.
function(_qt_internal_copy_file_if_different_command out_var src_file dst_file)
    # The CMake version higher than 3.23 doesn't contain the issue
    if(CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_LESS 3.23)
        set(${out_var} "${CMAKE_COMMAND}"
            "-DSRC_FILE_PATH=${src_file}"
            "-DDST_FILE_PATH=${dst_file}"
            -P "${_qt_6_config_cmake_dir}/QtCopyFileIfDifferent.cmake"
            PARENT_SCOPE
        )
    else()
        set(${out_var} "${CMAKE_COMMAND}"
            -E copy_if_different
            "${src_file}"
            "${dst_file}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# The function checks if add_custom_command has the support of the DEPFILE argument.
function(_qt_internal_check_depfile_support out_var)
    if(CMAKE_GENERATOR MATCHES "Ninja" OR
        CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 AND CMAKE_GENERATOR MATCHES "Makefiles"
        OR CMAKE_VERSION VERSION_GREATER_EQUAL 3.21
        AND (CMAKE_GENERATOR MATCHES "Xcode"
            OR CMAKE_GENERATOR MATCHES "Visual Studio ([0-9]+)" AND CMAKE_MATCH_1 GREATER_EQUAL 12))
        set(${out_var} TRUE)
    else()
        set(${out_var} FALSE)
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()
