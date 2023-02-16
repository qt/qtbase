# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qprocess_and_guieventloop Test:
#####################################################################

qt_internal_add_test(tst_qprocess_and_guieventloop
    SOURCES
        tst_qprocess_and_guieventloop.cpp
    LIBRARIES
        Qt::Gui
)
add_subdirectory(write-read-write)
add_dependencies(tst_qprocess_and_guieventloop write-read-write)
