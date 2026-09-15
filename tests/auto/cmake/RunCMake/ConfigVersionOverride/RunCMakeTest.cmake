# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(override_success ${cmake_opts})

run_cmake_with_options(break_early ${cmake_opts})

run_cmake_with_options(no_config_version_override_files ${cmake_opts})

run_cmake_with_options(multiple_components_for_base_bar_foo ${cmake_opts})
run_cmake_with_options(multiple_components_for_base_foo_bar ${cmake_opts})
run_cmake_with_options(multiple_components_for_base_bar_foo_required ${cmake_opts})
run_cmake_with_options(multiple_components_for_base_foo_bar_required ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "tries to override TestPkg with version 6.130 for component Boo")
run_cmake_with_options(override_files_conflict ${cmake_opts})

set(RunCMake_TEST_EXPECT_stderr "Could not find a configuration file for package \"TestPkg\"")
run_cmake_with_options(multiple_components_conflict_foo_bar ${cmake_opts})
run_cmake_with_options(multiple_components_conflict_bar_foo ${cmake_opts})

set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "Could not find a configuration file for package \"TestPkg\"")
run_cmake_with_options(multiple_components_conflict_foo_bar_required ${cmake_opts})
run_cmake_with_options(multiple_components_conflict_bar_foo_required ${cmake_opts})
unset(RunCMake_TEST_EXPECT_RESULT)

set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "are missing in")
run_cmake_with_options(missing_override_files ${cmake_opts})
unset(RunCMake_TEST_EXPECT_RESULT)

set(RunCMake_TEST_EXPECT_stderr "Could not find a configuration file for package \"TestPkg\"")
run_cmake_with_options(inconsistent_override_files ${cmake_opts})
run_cmake_with_options(dependency_version_conflict_foo_bar ${cmake_opts})
run_cmake_with_options(dependency_version_conflict_bar_foo ${cmake_opts})

set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "Could not find a configuration file for package \"TestPkg\"")
run_cmake_with_options(dependency_version_conflict_foo_boo_bar_required ${cmake_opts})
run_cmake_with_options(dependency_version_conflict_bar_foo_boo_required ${cmake_opts})
unset(RunCMake_TEST_EXPECT_RESULT)

set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "Expected TestPkg to be found via version override")
run_cmake_with_options(override_fail ${cmake_opts})


