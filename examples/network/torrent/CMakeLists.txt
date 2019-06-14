# Generated from torrent.pro.

cmake_minimum_required(VERSION 3.14)
project(torrent LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(torrent
    addtorrentdialog.cpp addtorrentdialog.h
    bencodeparser.cpp bencodeparser.h
    connectionmanager.cpp connectionmanager.h
    filemanager.cpp filemanager.h
    forms/addtorrentform.ui
    icons.qrc
    main.cpp
    mainwindow.cpp mainwindow.h
    metainfo.cpp metainfo.h
    peerwireclient.cpp peerwireclient.h
    ratecontroller.cpp ratecontroller.h
    torrentclient.cpp torrentclient.h
    torrentserver.cpp torrentserver.h
    trackerclient.cpp trackerclient.h
)
target_include_directories(torrent PUBLIC forms) # special case
target_link_libraries(torrent PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS torrent
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
