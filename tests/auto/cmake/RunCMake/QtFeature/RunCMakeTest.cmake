# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

run_cmake_with_options(feature_resolution "-DQt6_DIR=${Qt6_DIR}")
