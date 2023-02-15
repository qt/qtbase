# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcommandlineparser_test_helper Binary:
#####################################################################

qt_internal_add_executable(qcommandlineparser_test_helper
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qcommandlineparser_test_helper.cpp
)
