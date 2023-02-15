# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

## Scopes:
#####################################################################

qt_internal_extend_target(qcursorhighdpi CONDITION WIN32
    LIBRARIES
        user32
)
