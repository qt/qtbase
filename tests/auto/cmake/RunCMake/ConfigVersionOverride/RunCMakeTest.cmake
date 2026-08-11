# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(override_success ${cmake_opts})

run_cmake_with_options(break_early ${cmake_opts})

run_cmake_with_options(no_config_version_override_files ${cmake_opts})

set(RunCMake_TEST_EXPECT_RESULT 1)

set(RunCMake_TEST_EXPECT_stderr "matches requested version \"6.140\"")
run_cmake_with_options(override_fail ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "however it is not allowed")
run_cmake_with_options(multiple_components_conflict_foo_bar ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "but it set TestPkg_FOUND to FALSE")
run_cmake_with_options(multiple_components_conflict_bar_foo ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "are missing in")
run_cmake_with_options(missing_override_files ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "however version was set to")
run_cmake_with_options(inconsistent_override_files ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "The find_package call tries to override")
run_cmake_with_options(dependency_version_conflict_foo_bar ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "but it set TestPkg_FOUND to FALSE")
run_cmake_with_options(dependency_version_conflict_bar_foo ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "The find_package call tries to override")
run_cmake_with_options(dependency_version_conflict_foo_boo_bar ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "but it set TestPkg_FOUND to FALSE")
run_cmake_with_options(dependency_version_conflict_bar_foo_boo ${cmake_opts})


