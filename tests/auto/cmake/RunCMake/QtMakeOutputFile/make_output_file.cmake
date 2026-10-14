# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(source_dir "${CMAKE_CURRENT_BINARY_DIR}/src")
set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}/bin")

function(check_output_file infile prefix suffix expected)
    qt_make_output_file("${infile}" "${prefix}" "${suffix}" "${source_dir}" "${binary_dir}" outfile)
    if(NOT outfile STREQUAL expected)
        message(FATAL_ERROR
            "qt_make_output_file(${infile}) returned '${outfile}', expected '${expected}'")
    endif()
endfunction()

check_output_file("foo.cpp" "" ".mapped" "${binary_dir}/foo.mapped")
check_output_file("sub/foo.cpp" "prefix_" ".cpp" "${binary_dir}/sub/prefix_foo.cpp")

# Leading "../" escapes the source dir and is mapped to "__/" to stay inside the binary dir.
check_output_file("../foo.cpp" "prefix_" ".cpp" "${binary_dir}/__/prefix_foo.cpp")

# An input that already lives in the binary dir is relative to it, not to the source dir.
check_output_file("${binary_dir}/foo.cpp" "prefix_" ".cpp" "${binary_dir}/prefix_foo.cpp")
