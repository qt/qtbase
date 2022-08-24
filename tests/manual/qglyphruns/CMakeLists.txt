# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(qglyphrun
    GUI
    SOURCES
        main.cpp
        controller.h controller.cpp controller.ui
        view.h view.cpp
        glyphruninspector.h glyphruninspector.cpp
        singleglyphrun.h singleglyphrun.cpp singleglyphrun.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
