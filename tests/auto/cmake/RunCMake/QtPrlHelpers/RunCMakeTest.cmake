# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(find_package_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(qt_extract_library_name ${find_package_opts})
run_cmake_with_options(qt_finish_prl_file ${find_package_opts})
