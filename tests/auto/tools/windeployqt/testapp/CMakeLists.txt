# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from testapp.pro.

#####################################################################
## testapp Binary:
#####################################################################

qt_internal_add_executable(windeploy_testapp # special case
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)

# special case begin
set_target_properties(windeploy_testapp
    PROPERTIES
        OUTPUT_NAME testapp
)
# special case end
