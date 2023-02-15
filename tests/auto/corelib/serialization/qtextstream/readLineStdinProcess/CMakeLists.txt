# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## readLineStdinProcess Binary:
#####################################################################

qt_internal_add_executable(readLineStdinProcess
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qtextstream/readLineStdinProcess"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)
