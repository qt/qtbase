# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from dialogs.pro.

#####################################################################
## dialogs Binary:
#####################################################################

qt_internal_add_manual_test(dialogs
    GUI
    SOURCES
        colordialogpanel.cpp colordialogpanel.h
        filedialogpanel.cpp filedialogpanel.h
        fontdialogpanel.cpp fontdialogpanel.h
        main.cpp
        messageboxpanel.cpp messageboxpanel.h
        utils.cpp utils.h
        wizardpanel.cpp wizardpanel.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:dialogs.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(dialogs CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

qt_internal_extend_target(dialogs CONDITION NOT (QT_FEATURE_printer EQUAL FALSE)
    SOURCES
        printdialogpanel.cpp printdialogpanel.h printdialogpanel.ui
    ENABLE_AUTOGEN_TOOLS
        uic
)
