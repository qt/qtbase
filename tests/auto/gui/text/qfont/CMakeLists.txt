# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfont Test:
#####################################################################

# Resources:
set(testfont_resource_files
    "datastream.515"
    "weirdfont.otf"
)

qt_internal_add_test(tst_qfont
    SOURCES
        tst_qfont.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
    TESTDATA ${testfont_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfont CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
