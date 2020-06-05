# Generated from printsupport.pro.

#####################################################################
## PrintSupport Module:
#####################################################################

qt_add_module(PrintSupport
    PLUGIN_TYPES printsupport
    SOURCES
        kernel/qpaintengine_alpha.cpp kernel/qpaintengine_alpha_p.h
        kernel/qplatformprintdevice.cpp kernel/qplatformprintdevice.h
        kernel/qplatformprintersupport.cpp kernel/qplatformprintersupport.h
        kernel/qplatformprintplugin.cpp kernel/qplatformprintplugin.h
        kernel/qprint_p.h
        kernel/qprintdevice.cpp kernel/qprintdevice_p.h
        kernel/qprintengine.h
        kernel/qprintengine_pdf.cpp
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
)

#### Keys ignored in scope 1:.:.:printsupport.pro:<TRUE>:
# QMAKE_LIBS = "$$QMAKE_LIBS_PRINTSUPPORT"

## Scopes:
#####################################################################

qt_extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewwidget
    SOURCES
        kernel/qpaintengine_preview.cpp kernel/qpaintengine_preview_p.h
        widgets/qprintpreviewwidget.cpp widgets/qprintpreviewwidget.h
)

qt_extend_target(PrintSupport CONDITION WIN32
    SOURCES
        kernel/qprintengine_win.cpp kernel/qprintengine_win_p.h
    LIBRARIES
        comdlg32
        gdi32
        user32
        winspool
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_cups AND UNIX AND NOT APPLE
    SOURCES
        kernel/qcups.cpp kernel/qcups_p.h
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_cupsjobwidget AND UNIX AND NOT APPLE
    SOURCES
        widgets/qcupsjobwidget.cpp widgets/qcupsjobwidget.ui widgets/qcupsjobwidget_p.h
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog
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
        "images/fit-page-24.png"
        "images/fit-page-32.png"
        "images/fit-width-24.png"
        "images/fit-width-32.png"
        "images/go-first-24.png"
        "images/go-first-32.png"
        "images/go-last-24.png"
        "images/go-last-32.png"
        "images/go-next-24.png"
        "images/go-next-32.png"
        "images/go-previous-24.png"
        "images/go-previous-32.png"
        "images/layout-landscape-24.png"
        "images/layout-landscape-32.png"
        "images/layout-portrait-24.png"
        "images/layout-portrait-32.png"
        "images/page-setup-24.png"
        "images/page-setup-32.png"
        "images/print-24.png"
        "images/print-32.png"
        "images/view-page-multi-24.png"
        "images/view-page-multi-32.png"
        "images/view-page-one-24.png"
        "images/view-page-one-32.png"
        "images/view-page-sided-24.png"
        "images/view-page-sided-32.png"
        "images/zoom-in-24.png"
        "images/zoom-in-32.png"
        "images/zoom-out-24.png"
        "images/zoom-out-32.png"
    )

    qt_add_resource(PrintSupport "qprintdialog"
        PREFIX
            "/qt-project.org/dialogs/qprintpreviewdialog"
        BASE
            "dialogs"
        FILES
            ${qprintdialog_resource_files}
    )
    set(qprintdialog1_resource_files
        "images/status-color.png"
        "images/status-gray-scale.png"
    )

    qt_add_resource(PrintSupport "qprintdialog1"
        PREFIX
            "/qt-project.org/dialogs/qprintdialog"
        BASE
            "dialogs"
        FILES
            ${qprintdialog1_resource_files}
    )
endif()

qt_extend_target(PrintSupport CONDITION MACOS AND TARGET Qt::Widgets
    SOURCES
        platform/macos/qpaintengine_mac.mm platform/macos/qpaintengine_mac_p.h
        platform/macos/qcocoaprintdevice_p.h platform/macos/qcocoaprintdevice.mm
        platform/macos/qcocoaprintersupport_p.h platform/macos/qcocoaprintersupport.mm
        platform/macos/qprintengine_mac.mm platform/macos/qprintengine_mac_p.h
    NO_PCH_SOURCES
        "platform/macos/qcocoaprintersupport.mm"
    PUBLIC_LIBRARIES
        Qt::WidgetsPrivate
        Cups::Cups
)

qt_extend_target(PrintSupport CONDITION MACOS AND QT_FEATURE_printdialog
    SOURCES
        dialogs/qpagesetupdialog_mac.mm
        dialogs/qprintdialog_mac.mm
    LIBRARIES
        ${FWAppKit}
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND WIN32
    SOURCES
        dialogs/qpagesetupdialog_win.cpp
        dialogs/qprintdialog_win.cpp
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND UNIX AND NOT APPLE
    SOURCES
        dialogs/qpagesetupdialog_unix.cpp dialogs/qpagesetupdialog_unix_p.h
        dialogs/qprintdialog_unix.cpp
        dialogs/qprintpropertieswidget.ui
        dialogs/qprintsettingsoutput.ui
        dialogs/qprintwidget.ui
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/plugins/printsupport/cups
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewdialog
    SOURCES
        dialogs/qprintpreviewdialog.cpp dialogs/qprintpreviewdialog.h
)
qt_add_docs(PrintSupport
    doc/qtprintsupport.qdocconf
)

