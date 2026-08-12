# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(gui_snippets OBJECT
    brush/brush.cpp
    brush/gradientcreationsnippet.cpp
    image/image.cpp
    image/supportedformat.cpp
    polygon/polygon.cpp
    qimagewriter/main.cpp
    qtextobject/textobjectinterface.h
    rhioffscreen/main.cpp
    textdocument-end/textdocumentendsnippet.cpp
)

target_link_libraries(gui_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
)


if(QT_FEATURE_picture)
    target_sources(gui_snippets PRIVATE
        picture/picture.cpp
    )
endif()

if(QT_FEATURE_widgets)
    target_link_libraries(gui_snippets PRIVATE
        Qt::Widgets
    )
    target_sources(gui_snippets PRIVATE
        draganddrop/dragwidget.cpp
        dragging/mainwindow.cpp
        droparea/droparea.cpp
        dropevents/window.cpp
        droprectangle/window.cpp
        plaintextlayout/window.cpp
        qfileopenevent/main.cpp
        qfontdatabase/qfontdatabase_snippets.cpp
        qstatustipevent/main.cpp
        scribe-overview/main.cpp
        separations/finalwidget.cpp
        textblock-formats/main.cpp
        textdocument-charformats/main.cpp
        textdocument-css/main.cpp
        textdocument-cursors/main.cpp
        textdocument-find/main.cpp
        textdocument-imageformat/main.cpp
        textdocument-images/main.cpp
        textdocument-lists/mainwindow.cpp
        textdocument-resources/main.cpp
        transform/main.cpp
    )
    if(QT_FEATURE_shortcut)
        target_sources(gui_snippets PRIVATE
            textdocument-blocks/mainwindow.cpp
            textdocument-frames/mainwindow.cpp
            textdocument-listitems/mainwindow.cpp
            textdocument-listitemstyles/mainwindow.cpp
            textdocument-selections/mainwindow.cpp
            textdocument-tables/mainwindow.cpp
        )
    endif()
endif()


if(QT_FEATURE_xml)
    target_link_libraries(gui_snippets PRIVATE
        Qt::Xml
    )
    target_sources(gui_snippets PRIVATE
        textblock-fragments/xmlwriter.cpp
    )
endif()

if(QT_FEATURE_printsupport)
    target_link_libraries(gui_snippets PRIVATE
        Qt::PrintSupport
    )
    if(QT_FEATURE_shortcut)
        target_sources(gui_snippets PRIVATE
            textdocument-printing/mainwindow.cpp
        )
    endif()
endif()

if(QT_FEATURE_widgets AND QT_FEATURE_clipboard)
    target_sources(gui_snippets PRIVATE
        clipboard/clipwindow.cpp
    )
endif()

set_target_properties(gui_snippets PROPERTIES COMPILE_OPTIONS "-w")

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET gui_snippets PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(gui_snippets PROPERTIES UNITY_BUILD OFF)

add_subdirectory(code)
