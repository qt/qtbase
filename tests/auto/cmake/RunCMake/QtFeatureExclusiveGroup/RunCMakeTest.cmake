# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(find_package_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(exclusive_group_pass ${find_package_opts})

set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "Features group_a, group_b cannot be enabled together")
run_cmake_with_options(exclusive_group_fail ${find_package_opts})

# An explicit feature value overrides the computed default.
set(RunCMake_TEST_EXPECT_RESULT 1)
set(RunCMake_TEST_EXPECT_stderr "Features group_a, group_b cannot be enabled together")
run_cmake_with_options(exclusive_group_fail_cache ${find_package_opts} -DFEATURE_group_b=ON)
