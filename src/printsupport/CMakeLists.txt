# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## PrintSupport Module:
#####################################################################

qt_internal_add_module(PrintSupport
    PLUGIN_TYPES printsupport
    SOURCES
        kernel/qpaintengine_alpha.cpp kernel/qpaintengine_alpha_p.h
        kernel/qplatformprintdevice.cpp kernel/qplatformprintdevice.h
        kernel/qplatformprintersupport.cpp kernel/qplatformprintersupport.h
        kernel/qplatformprintplugin.cpp kernel/qplatformprintplugin.h
        kernel/qprint.cpp kernel/qprint_p.h
        kernel/qprintdevice.cpp kernel/qprintdevice_p.h
        kernel/qprintengine.h
        kernel/qprintengine_pdf.cpp kernel/qprintengine_pdf_p.h
        kernel/qprinter.cpp kernel/qprinter.h kernel/qprinter_p.h
        kernel/qprinterinfo.cpp kernel/qprinterinfo.h kernel/qprinterinfo_p.h
        kernel/qtprintsupportglobal.h kernel/qtprintsupportglobal_p.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    INCLUDE_DIRECTORIES
        dialogs
        widgets
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    GENERATE_CPP_EXPORTS
    QPA_HEADER_FILTERS
        "(^|/)qplatform.+\\.h$"
)

if(WIN32) # Static plugin link errors
    set_target_properties(PrintSupport PROPERTIES UNITY_BUILD OFF)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(PrintSupport CONDITION MACOS
    SOURCES
        platform/macos/qcocoaprintdevice.mm platform/macos/qcocoaprintdevice_p.h
        platform/macos/qcocoaprintersupport.mm platform/macos/qcocoaprintersupport_p.h
        platform/macos/qpaintengine_mac.mm platform/macos/qpaintengine_mac_p.h
        platform/macos/qprintengine_mac.mm platform/macos/qprintengine_mac_p.h
    PUBLIC_LIBRARIES
        ${FWApplicationServices}
        Cups::Cups
    NO_PCH_SOURCES
        platform/macos/qcocoaprintersupport.mm
)

qt_internal_extend_target(PrintSupport CONDITION WIN32
    SOURCES
        platform/windows/qprintengine_win.cpp platform/windows/qprintengine_win_p.h
        platform/windows/qwindowsprintdevice.cpp platform/windows/qwindowsprintdevice_p.h
        platform/windows/qwindowsprintersupport.cpp platform/windows/qwindowsprintersupport_p.h
    LIBRARIES
        gdi32
        user32
    PUBLIC_LIBRARIES
        comdlg32
        winspool
    NO_PCH_SOURCES
        platform/windows/qwindowsprintersupport.cpp
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewwidget
    SOURCES
        kernel/qpaintengine_preview.cpp kernel/qpaintengine_preview_p.h
        widgets/qprintpreviewwidget.cpp widgets/qprintpreviewwidget.h
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_cups AND UNIX AND NOT APPLE
    SOURCES
        kernel/qcups.cpp kernel/qcups_p.h
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_cupsjobwidget AND UNIX AND NOT APPLE
    SOURCES
        widgets/qcupsjobwidget.cpp widgets/qcupsjobwidget.ui widgets/qcupsjobwidget_p.h
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog
    SOURCES
        dialogs/qabstractprintdialog.cpp dialogs/qabstractprintdialog.h dialogs/qabstractprintdialog_p.h
        dialogs/qpagesetupdialog.cpp dialogs/qpagesetupdialog.h dialogs/qpagesetupdialog_p.h
        dialogs/qpagesetupwidget.ui
        dialogs/qprintdialog.h
    ENABLE_AUTOGEN_TOOLS
        uic
)

if(QT_FEATURE_printdialog)
    # Resources:
    set(qprintdialog_resource_files
        "dialogs/images/zoom-fit-page-24.png"
        "dialogs/images/zoom-fit-page-32.png"
        "dialogs/images/zoom-fit-width-24.png"
        "dialogs/images/zoom-fit-width-32.png"
        "dialogs/images/go-first-24.png"
        "dialogs/images/go-first-32.png"
        "dialogs/images/go-last-24.png"
        "dialogs/images/go-last-32.png"
        "dialogs/images/go-next-24.png"
        "dialogs/images/go-next-32.png"
        "dialogs/images/go-previous-24.png"
        "dialogs/images/go-previous-32.png"
        "dialogs/images/layout-landscape-24.png"
        "dialogs/images/layout-landscape-32.png"
        "dialogs/images/layout-portrait-24.png"
        "dialogs/images/layout-portrait-32.png"
        "dialogs/images/page-setup-24.png"
        "dialogs/images/page-setup-32.png"
        "dialogs/images/printer-24.png"
        "dialogs/images/printer-32.png"
        "dialogs/images/view-pages-overview-24.png"
        "dialogs/images/view-pages-overview-32.png"
        "dialogs/images/view-pages-single-24.png"
        "dialogs/images/view-pages-single-32.png"
        "dialogs/images/view-pages-facing-24.png"
        "dialogs/images/view-pages-facing-32.png"
        "dialogs/images/zoom-in-24.png"
        "dialogs/images/zoom-in-32.png"
        "dialogs/images/zoom-out-24.png"
        "dialogs/images/zoom-out-32.png"
    )

    qt_internal_add_resource(PrintSupport "qprintdialog"
        PREFIX
            "/qt-project.org/dialogs/qprintpreviewdialog"
        BASE
            "dialogs"
        FILES
            ${qprintdialog_resource_files}
    )
    set(qprintdialog1_resource_files
        "dialogs/images/status-color.png"
        "dialogs/images/status-gray-scale.png"
    )

    qt_internal_add_resource(PrintSupport "qprintdialog1"
        PREFIX
            "/qt-project.org/dialogs/qprintdialog"
        BASE
            "dialogs"
        FILES
            ${qprintdialog1_resource_files}
    )
endif()

qt_internal_extend_target(PrintSupport CONDITION MACOS AND QT_FEATURE_printdialog
    SOURCES
        dialogs/qpagesetupdialog_mac.mm
        dialogs/qprintdialog_mac.mm
    LIBRARIES
        ${FWAppKit}
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND WIN32
    SOURCES
        dialogs/qpagesetupdialog_win.cpp
        dialogs/qprintdialog_win.cpp
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND UNIX AND NOT APPLE
    SOURCES
        dialogs/qpagesetupdialog_unix.cpp dialogs/qpagesetupdialog_unix_p.h
        dialogs/qprintdialog_unix.cpp
        dialogs/qprintpropertieswidget.ui
        dialogs/qprintsettingsoutput.ui
        dialogs/qprintwidget.ui
    NO_UNITY_BUILD_SOURCES
        dialogs/qprintdialog_unix.cpp   # Clashes with CUPS headers
    INCLUDE_DIRECTORIES
        ${QtBase_SOURCE_DIR}/src/plugins/printsupport/cups
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_internal_extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewdialog
    SOURCES
        dialogs/qprintpreviewdialog.cpp dialogs/qprintpreviewdialog.h
)
qt_internal_add_docs(PrintSupport
    doc/qtprintsupport.qdocconf
)
