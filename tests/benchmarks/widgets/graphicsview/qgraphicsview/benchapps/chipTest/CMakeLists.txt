# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## chip Binary:
#####################################################################

qt_internal_add_benchmark(chip
    GUI
    SOURCES
        chip.cpp chip.h
        main.cpp
        mainwindow.cpp mainwindow.h
        view.cpp view.h
    LIBRARIES
        Qt::Gui
)

# Resources:
set(images_resource_files
    "fileprint.png"
    "qt4logo.png"
    "rotateleft.png"
    "rotateright.png"
    "zoomin.png"
    "zoomout.png"
)

qt_internal_add_resource(chip "images"
    PREFIX
        "/"
    FILES
        ${images_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(chip CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
)
