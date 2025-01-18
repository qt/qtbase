# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_executable(silent_fatal
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_silent_fatal.cpp
        ../../../../shared/disablecoredumps.cpp
    LIBRARIES
        Qt::TestPrivate
)

# No coverage because this crashes, making data collection difficult, if not
# impossible.
