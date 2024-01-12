# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qimagereader Binary:
#####################################################################

qt_internal_add_manual_test(qimagereader
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
set(qimagereader_resource_files
    "Qt_logostrap_CMYK.jpg"
)
qt_add_resources(qimagereader "qimagereader"
    PREFIX
        "/"
    FILES
        ${qimagereader_resource_files}
)
