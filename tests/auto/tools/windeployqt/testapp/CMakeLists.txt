# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## testapp Binary:
#####################################################################

qt_internal_add_executable(windeploy_testapp
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)

set_target_properties(windeploy_testapp
    PROPERTIES
        OUTPUT_NAME testapp
)
