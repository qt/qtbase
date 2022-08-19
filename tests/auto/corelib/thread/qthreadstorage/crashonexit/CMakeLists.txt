# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## crashonexit Binary:
#####################################################################

qt_internal_add_executable(crashOnExit_helper
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qthreadstorage/crashOnExit_helper"
    SOURCES
        crashOnExit.cpp
)
