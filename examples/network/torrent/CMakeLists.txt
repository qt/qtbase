# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(torrent LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/torrent")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets)

qt_standard_project_setup()

qt_add_executable(torrent
    addtorrentdialog.cpp addtorrentdialog.h
    addtorrentform.ui
    bencodeparser.cpp bencodeparser.h
    connectionmanager.cpp connectionmanager.h
    filemanager.cpp filemanager.h
    main.cpp
    mainwindow.cpp mainwindow.h
    metainfo.cpp metainfo.h
    peerwireclient.cpp peerwireclient.h
    ratecontroller.cpp ratecontroller.h
    torrentclient.cpp torrentclient.h
    torrentserver.cpp torrentserver.h
    trackerclient.cpp trackerclient.h
)

set_target_properties(torrent PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(torrent PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
)

# Resources:
set(icons_resource_files
    "icons/1downarrow.png"
    "icons/1uparrow.png"
    "icons/bottom.png"
    "icons/exit.png"
    "icons/peertopeer.png"
    "icons/player_pause.png"
    "icons/player_play.png"
    "icons/player_stop.png"
)

qt_add_resources(torrent "icons"
    PREFIX
        "/"
    FILES
        ${icons_resource_files}
)

install(TARGETS torrent
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
