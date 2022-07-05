# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qcursorhighdpi.pro.

#####################################################################
## qcursorhighdpi Binary:
#####################################################################

qt_internal_add_manual_test(qcursorhighdpi
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qcursorhighdpi.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(qcursorhighdpi CONDITION WIN32
    LIBRARIES
        user32
)
