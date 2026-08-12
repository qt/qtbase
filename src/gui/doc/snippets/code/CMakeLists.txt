# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(gui_snippets_code OBJECT
    src_gui_accessible_qaccessible.cpp
    src_gui_image_qimage.cpp
    src_gui_image_qimagereader.cpp
    src_gui_image_qimagewriter.cpp
    src_gui_image_qpixmap.cpp
    src_gui_image_qpixmapcache.cpp
    src_gui_kernel_qguiapplication_x11.cpp
    src_gui_math3d_qquaternion.cpp
    src_gui_painting_qbrush.cpp
    src_gui_painting_qcolor.cpp
    src_gui_painting_qpainterpath.cpp
    src_gui_painting_qpen.cpp
    src_gui_painting_qregion_unix.cpp
    src_gui_painting_qregion.cpp
    src_gui_painting_qpen.cpp
    src_gui_painting_qregion_unix.cpp
    src_gui_painting_qregion.cpp
    src_gui_text_qfont.cpp
    src_gui_text_qfontmetrics.cpp
    src_gui_text_qtextcursor.cpp
    src_gui_text_qtextdocumentwriter.cpp
    src_gui_text_qtextlayout.cpp
    src_gui_util_qdesktopservices.cpp
    src_gui_util_qundostack.cpp
)

target_link_libraries(gui_snippets_code PRIVATE
    Qt::Core
    Qt::Gui
)

if(QT_FEATURE_wheelevent)
    target_sources(gui_snippets_code PRIVATE
        src_gui_kernel_qevent.cpp
    )
endif()

if(QT_FEATURE_shortcut)
    target_sources(gui_snippets_code PRIVATE
        src_gui_kernel_qshortcut.cpp
        src_gui_kernel_qshortcutmap.cpp
    )
endif()

if(QT_FEATURE_widgets)
    target_link_libraries(gui_snippets_code PRIVATE
        Qt::Widgets
    )
    target_sources(gui_snippets_code PRIVATE
        doc_src_coordsys.cpp
        doc_src_richtext.cpp
        src_gui_image_qicon.cpp
        src_gui_image_qmovie.cpp
        src_gui_itemviews_qstandarditemmodel.cpp
        src_gui_kernel_qaction.cpp
        src_gui_kernel_qapplication.cpp
        src_gui_kernel_qguiapplication.cpp
        src_gui_text_qsyntaxhighlighter.cpp
        src_gui_util_qvalidator.cpp
    )
if(QT_FEATURE_shortcut)
    target_sources(gui_snippets_code PRIVATE
        src_gui_kernel_qkeysequence.cpp
    )
endif()
if(QT_FEATURE_opengl)
    target_sources(gui_snippets PRIVATE
        src_gui_opengl_qopenglfunctions.cpp
        src_gui_painting_qpainter.cpp
    )
endif()
endif()

if(QT_FEATURE_vulkan)
    target_sources(gui_snippets_code PRIVATE
        src_gui_vulkan_qvulkanfunctions.cpp
        src_gui_vulkan_qvulkaninstance.cpp
        src_gui_vulkan_qvulkanwindow.cpp
    )
endif()

if(QT_FEATURE_clipboard)
    target_sources(gui_snippets_code PRIVATE
        src_gui_kernel_qclipboard.cpp
    )
endif()

set_target_properties(gui_snippets_code PROPERTIES COMPILE_OPTIONS "-w")

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET gui_snippets_code PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(gui_snippets_code PROPERTIES UNITY_BUILD OFF)
