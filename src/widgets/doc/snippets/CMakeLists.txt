# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(widgets_snippets OBJECT
    customstyle/customstyle.cpp
    customstyle/main.cpp
    customviewstyle/customviewstyle.cpp
    dialogs/dialogs.cpp
    itemselection/main.cpp
    layouts/layouts.cpp
    qlistview-dnd/model.cpp
    qrhiwidget/rhiwidgetintro.cpp
    qsortfilterproxymodel/main.cpp
    qsplashscreen/main.cpp
    qstackedlayout/main.cpp
    qstackedwidget/main.cpp
    qstyleoption/main.cpp
    qstyleplugin/main.cpp
    reading-selections/window.cpp
    shareddirmodel/main.cpp
    simplemodel-use/main.cpp
    splitter/splitter.cpp
    stringlistmodel/main.cpp
    stringlistmodel/model.cpp
    styles/styles.cpp
    textdocument-imagedrop/textedit.cpp
    timeline/main.cpp
    tooltips/main.cpp
    updating-selections/window.cpp
    widget-mask/main.cpp
    widgets-tutorial/template.cpp
    mainwindowsnippet.cpp
    widgetdelegate.cpp
)

target_link_libraries(widgets_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Widgets
)

if(QT_FEATURE_shortcut)
    target_sources(widgets_snippets PRIVATE
        dockwidgets/mainwindow.cpp
        qlistview-dnd/mainwindow.cpp
        qlistwidget-dnd/mainwindow.cpp
        qtablewidget-resizing/mainwindow.cpp
        qtablewidget-using/mainwindow.cpp
        qtreewidget-using/mainwindow.cpp
        qtreewidgetitemiterator-using/mainwindow.cpp
        whatsthis/whatsthis.cpp
    )
endif()

if(QT_FEATURE_filedialog)
    target_sources(widgets_snippets PRIVATE
        filedialogurls/filedialogurls.cpp
    )
endif()

if(QT_FEATURE_graphicsview)
    target_sources(widgets_snippets PRIVATE
        graphicssceneadditem/graphicssceneadditemsnippet.cpp
        graphicsview/graphicsview_snippet.cpp
    )
endif()

if(QT_FEATURE_opengl AND QT_FEATURE_printsupport)
    target_link_libraries(widgets_snippets PRIVATE
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::PrintSupport
    )
    target_sources(widgets_snippets PRIVATE
        graphicsview/graphicsview.cpp
    )
endif()

if(QT_FEATURE_mdiarea)
    target_sources(widgets_snippets PRIVATE
        mdiarea/mdiareasnippets.cpp
    )
endif()

if(QT_FEATURE_scrollarea)
    target_sources(widgets_snippets PRIVATE
        myscrollarea/myscrollarea.cpp
    )
endif()

if(QT_FEATURE_spinbox AND QT_FEATURE_itemviews)
    target_sources(widgets_snippets PRIVATE
        qitemdelegate/spinbox-delegate.cpp
    )
endif()

set_target_properties(widgets_snippets PROPERTIES COMPILE_OPTIONS "-w")

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET widgets_snippets PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(widgets_snippets PROPERTIES UNITY_BUILD OFF)

add_subdirectory(code)
