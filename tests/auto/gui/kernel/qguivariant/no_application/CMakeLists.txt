# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## no_application Test:
#####################################################################

qt_internal_add_test(tst_gui_variant_no_application
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)
