# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from testProcessUniqueness.pro.

#####################################################################
## testProcessUniqueness Binary:
#####################################################################

qt_internal_add_executable(testProcessUniqueness
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_quuid/testProcessUniqueness"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)

# special case begin
set_target_properties(testProcessUniqueness PROPERTIES MACOSX_BUNDLE TRUE)
# special case end
