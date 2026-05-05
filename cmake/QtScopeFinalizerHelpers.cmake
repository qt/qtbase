# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Add a finalizer function for the current CMake list file.
# It will be processed just before leaving the current source directory scope.
function(qt_add_list_file_finalizer func)
    cmake_language(EVAL CODE "cmake_language(DEFER CALL \"${func}\" ${ARGN}) ")
endfunction()
