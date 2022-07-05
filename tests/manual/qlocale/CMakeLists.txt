# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qlocale.pro.

#####################################################################
## qlocale Binary:
#####################################################################

qt_internal_add_manual_test(qlocale
    GUI
    SOURCES
        calendar.cpp calendar.h
        currency.cpp currency.h
        dateformats.cpp dateformats.h
        info.cpp info.h
        languages.cpp languages.h
        main.cpp
        miscellaneous.cpp miscellaneous.h
        numberformats.cpp numberformats.h
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
