
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
function(check_parameters file_path)
    file(READ ${file_path} file_content)
    foreach(compile_option IN ITEMS "-DDEFINE_CMDLINE_SIGNAL" "-DMY_OPTION")
        string(REGEX MATCHALL "${compile_option}" matches ${file_content})
        list(LENGTH matches matches_length)
        if(matches_length GREATER 1)
            message(FATAL_ERROR "${compile_option} is defined multiple times in ${file_path}")
        elseif(matches_length EQUAL 0)
            message(FATAL_ERROR "${compile_option} is not defined in ${file_path}")
        endif()
    endforeach()
endfunction()

check_parameters(${PARAMETERS_FILE_PATH})
