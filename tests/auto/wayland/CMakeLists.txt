# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from client.pro.

add_subdirectory(shared)

# webOS has a modified version of QtWayland and does not support e.g. multiple window creation
# in a single client, attempting to do so will cause a segmentation fault
if (NOT WEBOS)
    add_subdirectory(client)
    add_subdirectory(clientextension)
    add_subdirectory(cursor)
    add_subdirectory(datadevicev1)
    add_subdirectory(fullscreenshellv1)
    add_subdirectory(nooutput)
    add_subdirectory(output)
    add_subdirectory(primaryselectionv1)
    add_subdirectory(reconnect)
    add_subdirectory(seatv4)
    add_subdirectory(seatv7)
    add_subdirectory(seat)
    add_subdirectory(surface)
    add_subdirectory(tabletv2)
    add_subdirectory(wl_connect)
    add_subdirectory(xdgdecorationv1)
    add_subdirectory(xdgoutput)
    add_subdirectory(xdgshell)
    add_subdirectory(scaling)
endif()
add_subdirectory(multithreaded)

if(QT_FEATURE_im)
    add_subdirectory(inputcontext)
    add_subdirectory(textinputv3)
endif()
