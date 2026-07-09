# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
# Reuse the Linux compiler and compiler-wrapper configuration files and set
# UNIX/LINUX, which are cleared before platform initialization.
set(CMAKE_EFFECTIVE_SYSTEM_NAME "Linux")
include(Platform/Linux-Initialize)
