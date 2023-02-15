# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qkeysequence Test:
#####################################################################

# Resources:
set(qkeysequence_resource_files
    "keys_de.qm"
    "qt_de.qm"
)

qt_internal_add_test(tst_qkeysequence
    SOURCES
        tst_qkeysequence.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qkeysequence_resource_files}
    BUILTIN_TESTDATA
)
