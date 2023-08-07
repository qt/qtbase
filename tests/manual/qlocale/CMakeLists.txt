# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
    NO_PCH_SOURCES
        languages.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
