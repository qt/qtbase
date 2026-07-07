# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

function(run_cmake_and_build case)
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    run_cmake_with_options(${case} ${cmake_opts})
    # Do not remove the current RunCMake_TEST_BINARY_DIR
    set(RunCMake_TEST_NO_CLEAN 1)
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

# Like run_cmake_and_build, but the build step is expected to fail.
function(run_cmake_and_build_expect_fail case)
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    run_cmake_with_options(${case} ${cmake_opts})
    set(RunCMake_TEST_NO_CLEAN 1)
    set(RunCMake_TEST_OUTPUT_MERGE 1)
    set(RunCMake_TEST_EXPECT_RESULT "[^0]")
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

# moc_<base>.cpp #include'd by the source (goes to <autogen>/include)

# A single source/header pair in a subdir.
run_cmake_and_build(single_source)

# Two same-named pairs in a parent and a child directory.
run_cmake_and_build(same_name_parent_child)

# Two same-named pairs in sibling subdirectories.
run_cmake_and_build(same_name_siblings)

# Two same-named sources #include the *bare* "moc_foo.cpp". The includes are
# ambiguous, so AUTOMOC fails at build time.
run_cmake_and_build_expect_fail(same_include_string)

# moc_<base>.cpp NOT #include'd, auto-added to mocs_compilation.cpp

# A single source/header pair in a subdir, no moc includes.
run_cmake_and_build(mocs_compilation)

# Two same-named pairs in sibling subdirectories, two distinct entries in mocs_compilation.cpp
run_cmake_and_build(mocs_compilation_same_name)

# Q_OBJECT declared in a .cpp (<base>.moc)

# A single source file that #include's its foo.moc.
run_cmake_and_build(dot_moc)

# Two same-named source files each #include's its own .moc.
run_cmake_and_build(dot_moc_same_name)

# A source file that omits its foo.moc include, AUTOMOC fails.
run_cmake_and_build_expect_fail(dot_moc_missing)
