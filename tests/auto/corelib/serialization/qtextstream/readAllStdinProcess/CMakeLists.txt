# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## readAllStdinProcess Binary:
#####################################################################

qt_internal_add_executable(readAllStdinProcess
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qtextstream/readAllStdinProcess"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)
