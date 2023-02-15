# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## hellominimalcrossgfxtriangle Binary:
#####################################################################

qt_internal_add_manual_test(hellominimalcrossgfxtriangle
    SOURCES
        hellowindow.cpp hellowindow.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set(hellominimalcrossgfxtriangle_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
)

qt_internal_add_resource(hellominimalcrossgfxtriangle "hellominimalcrossgfxtriangle"
    PREFIX
        "/"
    FILES
        ${hellominimalcrossgfxtriangle_resource_files}
)
