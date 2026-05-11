# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The OHOS SDK sets CMAKE_*_COMPILER_EXTERNAL_TOOLCHAIN, causing CMake to
# inject --gcc-toolchain=<path> into every compile command.  OHOS Clang does
# not use a GCC toolchain for aarch64-linux-ohos, so the flag is unused and
# triggers -Wunused-command-line-argument.
unset(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN)
unset(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN)
unset(CMAKE_ASM_COMPILER_EXTERNAL_TOOLCHAIN)

# OHOS does not use versioned shared library names.
set(CMAKE_PLATFORM_NO_VERSIONED_SONAME ON)
