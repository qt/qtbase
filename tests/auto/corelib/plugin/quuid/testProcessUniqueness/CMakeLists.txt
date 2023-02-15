# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testProcessUniqueness Binary:
#####################################################################

qt_internal_add_executable(testProcessUniqueness
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_quuid/testProcessUniqueness"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)

set_target_properties(testProcessUniqueness PROPERTIES MACOSX_BUNDLE TRUE)
