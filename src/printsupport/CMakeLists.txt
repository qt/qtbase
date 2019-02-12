# Generated from printsupport.pro.

#####################################################################
## PrintSupport Module:
#####################################################################

add_qt_module(PrintSupport
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
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        widgets
        dialogs
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES Qt::Gui # manual: for QPageSize
    # MODULE_PLUGIN_TYPES = "printsupport"
    # QMAKE_LIBS = "$$QMAKE_LIBS_PRINTSUPPORT"
    # _LOADED = "qt_module"
)

## Scopes:
#####################################################################

extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewwidget
    SOURCES
        kernel/qpaintengine_preview.cpp kernel/qpaintengine_preview_p.h
        widgets/qprintpreviewwidget.cpp widgets/qprintpreviewwidget.h
)

extend_target(PrintSupport CONDITION WIN32
    SOURCES
        kernel/qprintengine_win.cpp kernel/qprintengine_win_p.h
)

extend_target(PrintSupport CONDITION WIN32 AND NOT WINRT
    LIBRARIES
        winspool
        comdlg32
        gdi32
        user32
)

extend_target(PrintSupport CONDITION QT_FEATURE_cups AND UNIX AND NOT APPLE
    SOURCES
        kernel/qcups.cpp kernel/qcups_p.h
)

extend_target(PrintSupport CONDITION QT_FEATURE_cupsjobwidget AND UNIX AND NOT APPLE
    SOURCES
        widgets/qcupsjobwidget.cpp widgets/qcupsjobwidget.ui widgets/qcupsjobwidget_p.h
)

extend_target(PrintSupport CONDITION QT_FEATURE_printdialog
    SOURCES
        dialogs/qabstractprintdialog.cpp dialogs/qabstractprintdialog.h dialogs/qabstractprintdialog_p.h
        dialogs/qpagesetupdialog.cpp dialogs/qpagesetupdialog.h dialogs/qpagesetupdialog_p.h
        dialogs/qpagesetupwidget.ui
        dialogs/qprintdialog.h dialogs/qprintdialog.qrc
)

extend_target(PrintSupport CONDITION APPLE_OSX AND QT_FEATURE_printdialog
    SOURCES
        dialogs/qpagesetupdialog_mac.mm
        dialogs/qprintdialog_mac.mm
    LIBRARIES
        ${FWAppKit}
)

extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND WIN32
    SOURCES
        dialogs/qpagesetupdialog_win.cpp
        dialogs/qprintdialog_win.cpp
)

extend_target(PrintSupport CONDITION QT_FEATURE_printdialog AND UNIX AND NOT APPLE
    SOURCES
        dialogs/qpagesetupdialog_unix.cpp dialogs/qpagesetupdialog_unix_p.h
        dialogs/qprintdialog_unix.cpp
        dialogs/qprintpropertieswidget.ui
        dialogs/qprintsettingsoutput.ui
        dialogs/qprintwidget.ui
    INCLUDE_DIRECTORIES
        ${PROJECT_SOURCE_DIR}/src/plugins/printsupport/cups
)

extend_target(PrintSupport CONDITION QT_FEATURE_printpreviewdialog
    SOURCES
        dialogs/qprintpreviewdialog.cpp dialogs/qprintpreviewdialog.h
)

add_qt_docs(./doc/qtprintsupport.qdocconf)
