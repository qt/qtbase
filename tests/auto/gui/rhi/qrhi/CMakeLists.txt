# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrhi Test:
#####################################################################

# Resources:
file(GLOB_RECURSE qrhi_resource_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    data/*
)

qt_internal_add_test(tst_qrhi
    SOURCES
        tst_qrhi.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qrhi_resource_files}
    BUILTIN_TESTDATA
)
