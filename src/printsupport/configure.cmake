# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries

qt_find_package(Cups PROVIDED_TARGETS Cups::Cups MODULE_NAME printsupport QMAKE_LIB cups)


#### Tests



#### Features

qt_feature("cups" PUBLIC PRIVATE
    SECTION "Painting"
    LABEL "CUPS"
    PURPOSE "Provides support for the Common Unix Printing System."
    CONDITION Cups_FOUND AND QT_FEATURE_printer AND QT_FEATURE_datestring
)
qt_feature_definition("cups" "QT_NO_CUPS" NEGATE VALUE "1")
qt_feature("cupsjobwidget" PUBLIC PRIVATE
    SECTION "Widgets"
    LABEL "CUPS job control widget"
    CONDITION ( QT_FEATURE_buttongroup ) AND ( QT_FEATURE_calendarwidget ) AND ( QT_FEATURE_checkbox ) AND ( QT_FEATURE_combobox ) AND ( QT_FEATURE_cups ) AND ( QT_FEATURE_datetimeedit ) AND ( QT_FEATURE_groupbox ) AND ( QT_FEATURE_tablewidget )
)
qt_feature_definition("cupsjobwidget" "QT_NO_CUPSJOBWIDGET" NEGATE VALUE "1")
qt_feature("cupspassworddialog" PRIVATE
    SECTION "Widgets"
    LABEL "CUPS password dialog"
    CONDITION ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_formlayout ) AND ( QT_FEATURE_lineedit )
)
qt_feature("printer" PUBLIC
    SECTION "Painting"
    LABEL "QPrinter"
    PURPOSE "Provides a printer backend of QPainter."
    CONDITION NOT UIKIT AND QT_FEATURE_picture AND QT_FEATURE_temporaryfile AND QT_FEATURE_pdf
)
qt_feature_definition("printer" "QT_NO_PRINTER" NEGATE VALUE "1")
qt_feature("printpreviewwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QPrintPreviewWidget"
    PURPOSE "Provides a widget for previewing page layouts for printer output."
    CONDITION QT_FEATURE_graphicsview AND QT_FEATURE_printer AND QT_FEATURE_mainwindow
)
qt_feature_definition("printpreviewwidget" "QT_NO_PRINTPREVIEWWIDGET" NEGATE VALUE "1")
qt_feature("printdialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QPrintDialog"
    PURPOSE "Provides a dialog widget for specifying printer configuration."
    CONDITION ( QT_FEATURE_buttongroup ) AND ( QT_FEATURE_checkbox ) AND ( QT_FEATURE_combobox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_datetimeedit ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_formlayout ) AND ( QT_FEATURE_printer ) AND ( QT_FEATURE_radiobutton ) AND ( QT_FEATURE_spinbox ) AND ( QT_FEATURE_tabwidget ) AND ( QT_FEATURE_treeview )
)
qt_feature_definition("printdialog" "QT_NO_PRINTDIALOG" NEGATE VALUE "1")
qt_feature("printpreviewdialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QPrintPreviewDialog"
    PURPOSE "Provides a dialog for previewing and configuring page layouts for printer output."
    CONDITION QT_FEATURE_printpreviewwidget AND QT_FEATURE_printdialog AND QT_FEATURE_toolbar AND QT_FEATURE_formlayout
)
qt_feature_definition("printpreviewdialog" "QT_NO_PRINTPREVIEWDIALOG" NEGATE VALUE "1")
qt_configure_add_summary_section(NAME "Qt PrintSupport")
qt_configure_add_summary_entry(ARGS "cups")
qt_configure_end_summary_section() # end of "Qt PrintSupport" section
