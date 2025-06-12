# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries

qt_find_package(GTK3 3.6 MODULE
    PROVIDED_TARGETS PkgConfig::GTK3 MODULE_NAME widgets QMAKE_LIB gtk3)


#### Tests



#### Features

qt_feature("gtk3" PRIVATE
    LABEL "GTK+"
    AUTODETECT NOT APPLE
    CONDITION QT_FEATURE_glib AND GTK3_FOUND
)
qt_feature("style-fusion" PRIVATE
    LABEL "Fusion"
)
qt_feature("style-mac" PRIVATE
    LABEL "macOS"
    CONDITION MACOS AND QT_FEATURE_animation
)
qt_feature("style-windows" PRIVATE
    LABEL "Windows"
)
qt_feature("style-windowsvista" PRIVATE
    LABEL "WindowsVista"
    CONDITION QT_FEATURE_style_windows AND QT_FEATURE_animation AND WIN32
)
qt_feature("style-windows11" PRIVATE
    LABEL "Windows11"
    CONDITION QT_FEATURE_style_windows AND QT_FEATURE_animation AND WIN32
)
qt_feature("style-android" PRIVATE
    LABEL "Android"
    AUTODETECT ANDROID
)
qt_feature("style-stylesheet" PUBLIC
    SECTION "Styles"
    LABEL "QStyleSheetStyle"
    PURPOSE "Provides a widget style which is configurable via CSS."
    CONDITION QT_FEATURE_style_windows AND QT_FEATURE_cssparser
)
qt_feature_definition("style-stylesheet" "QT_NO_STYLE_STYLESHEET" NEGATE VALUE "1")
qt_feature("effects" PRIVATE
    SECTION "Kernel"
    LABEL "Effects"
    PURPOSE "Provides special widget effects (e.g. fading and scrolling)."
)
qt_feature("itemviews" PUBLIC
    SECTION "ItemViews"
    LABEL "The Model/View Framework"
    PURPOSE "Provides the model/view architecture managing the relationship between data and the way it is presented to the user."
    CONDITION QT_FEATURE_itemmodel AND QT_FEATURE_scrollarea
)
qt_feature_definition("itemviews" "QT_NO_ITEMVIEWS" NEGATE VALUE "1")
qt_feature("treewidget" PUBLIC
    SECTION "Widgets"
    LABEL "QTreeWidget"
    PURPOSE "Provides views using tree models."
    CONDITION QT_FEATURE_treeview
)
qt_feature_definition("treewidget" "QT_NO_TREEWIDGET" NEGATE VALUE "1")
qt_feature("listwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QListWidget"
    PURPOSE "Provides item-based list widgets."
    CONDITION QT_FEATURE_listview
)
qt_feature_definition("listwidget" "QT_NO_LISTWIDGET" NEGATE VALUE "1")
qt_feature("tablewidget" PUBLIC
    SECTION "Widgets"
    LABEL "QTableWidget"
    PURPOSE "Provides item-based table views."
    CONDITION QT_FEATURE_tableview
)
qt_feature_definition("tablewidget" "QT_NO_TABLEWIDGET" NEGATE VALUE "1")
qt_feature("abstractbutton" PUBLIC
    SECTION "Widgets"
    LABEL "QAbstractButton"
    PURPOSE "Abstract base class of button widgets, providing functionality common to buttons."
)
qt_feature("commandlinkbutton" PUBLIC
    SECTION "Widgets"
    LABEL "QCommandLinkButton"
    PURPOSE "Provides a Vista style command link button."
    CONDITION QT_FEATURE_pushbutton
)
qt_feature("datetimeedit" PUBLIC
    SECTION "Widgets"
    LABEL "QDateTimeEdit"
    PURPOSE "Supports editing dates and times."
    CONDITION QT_FEATURE_calendarwidget AND QT_FEATURE_datetimeparser
)
qt_feature_definition("datetimeedit" "QT_NO_DATETIMEEDIT" NEGATE VALUE "1")
qt_feature("stackedwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QStackedWidget"
    PURPOSE "Provides stacked widgets."
)
qt_feature_definition("stackedwidget" "QT_NO_STACKEDWIDGET" NEGATE VALUE "1")
qt_feature("textbrowser" PUBLIC
    SECTION "Widgets"
    LABEL "QTextBrowser"
    PURPOSE "Supports HTML document browsing."
    CONDITION QT_FEATURE_textedit
)
qt_feature_definition("textbrowser" "QT_NO_TEXTBROWSER" NEGATE VALUE "1")
qt_feature("splashscreen" PUBLIC
    SECTION "Widgets"
    LABEL "QSplashScreen"
    PURPOSE "Supports splash screens that can be shown during application startup."
)
qt_feature_definition("splashscreen" "QT_NO_SPLASHSCREEN" NEGATE VALUE "1")
qt_feature("splitter" PUBLIC
    SECTION "Widgets"
    LABEL "QSplitter"
    PURPOSE "Provides user controlled splitter widgets."
)
qt_feature_definition("splitter" "QT_NO_SPLITTER" NEGATE VALUE "1")
qt_feature("widgettextcontrol" PRIVATE
    SECTION "Widgets"
    LABEL "QWidgetTextControl"
    PURPOSE "Provides text control functionality to other widgets."
)
qt_feature("label" PUBLIC
    SECTION "Widgets"
    LABEL "QLabel"
    PURPOSE "Provides a text or image display."
    CONDITION QT_FEATURE_widgettextcontrol
)
qt_feature("formlayout" PUBLIC
    SECTION "Widgets"
    LABEL "QFormLayout"
    PURPOSE "Manages forms of input widgets and their associated labels."
    CONDITION QT_FEATURE_label
)
qt_feature("lcdnumber" PUBLIC
    SECTION "Widgets"
    LABEL "QLCDNumber"
    PURPOSE "Provides LCD-like digits."
)
qt_feature_definition("lcdnumber" "QT_NO_LCDNUMBER" NEGATE VALUE "1")
qt_feature("menu" PUBLIC
    SECTION "Widgets"
    LABEL "QMenu"
    PURPOSE "Provides popup-menus."
    CONDITION QT_FEATURE_action AND QT_FEATURE_pushbutton
)
qt_feature_definition("menu" "QT_NO_MENU" NEGATE VALUE "1")
qt_feature("lineedit" PUBLIC
    SECTION "Widgets"
    LABEL "QLineEdit"
    PURPOSE "Provides single-line edits."
    CONDITION QT_FEATURE_widgettextcontrol
)
qt_feature_definition("lineedit" "QT_NO_LINEEDIT" NEGATE VALUE "1")
qt_feature("radiobutton" PUBLIC
    SECTION "Widgets"
    LABEL "QRadioButton"
    PURPOSE "Provides a radio button with a text label."
    CONDITION QT_FEATURE_abstractbutton
)
qt_feature("spinbox" PUBLIC
    SECTION "Widgets"
    LABEL "QSpinBox"
    PURPOSE "Provides spin boxes handling integers and discrete sets of values."
    CONDITION QT_FEATURE_lineedit AND QT_FEATURE_validator
)
qt_feature_definition("spinbox" "QT_NO_SPINBOX" NEGATE VALUE "1")
qt_feature("tabbar" PUBLIC
    SECTION "Widgets"
    LABEL "QTabBar"
    PURPOSE "Provides tab bars, e.g., for use in tabbed dialogs."
    CONDITION QT_FEATURE_toolbutton
)
qt_feature_definition("tabbar" "QT_NO_TABBAR" NEGATE VALUE "1")
qt_feature("tabwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QTabWidget"
    PURPOSE "Supports stacking tabbed widgets."
    CONDITION QT_FEATURE_tabbar AND QT_FEATURE_stackedwidget
)
qt_feature_definition("tabwidget" "QT_NO_TABWIDGET" NEGATE VALUE "1")
qt_feature("combobox" PUBLIC
    SECTION "Widgets"
    LABEL "QComboBox"
    PURPOSE "Provides drop-down boxes presenting a list of options to the user."
    CONDITION QT_FEATURE_lineedit AND QT_FEATURE_standarditemmodel AND QT_FEATURE_listview
)
qt_feature_definition("combobox" "QT_NO_COMBOBOX" NEGATE VALUE "1")
qt_feature("fontcombobox" PUBLIC
    SECTION "Widgets"
    LABEL "QFontComboBox"
    PURPOSE "Provides a combobox that lets the user select a font family."
    CONDITION QT_FEATURE_combobox AND QT_FEATURE_stringlistmodel
)
qt_feature_definition("fontcombobox" "QT_NO_FONTCOMBOBOX" NEGATE VALUE "1")
qt_feature("checkbox" PUBLIC
    SECTION "Widgets"
    LABEL "QCheckBox("
    PURPOSE "Provides a checkbox with a text label."
    CONDITION QT_FEATURE_abstractbutton
)
qt_feature("pushbutton" PUBLIC
    SECTION "Widgets"
    LABEL "QPushButton"
    PURPOSE "Provides a command button."
    CONDITION QT_FEATURE_abstractbutton AND QT_FEATURE_action
)
qt_feature("toolbutton" PUBLIC
    SECTION "Widgets"
    LABEL "QToolButton"
    PURPOSE "Provides quick-access buttons to commands and options."
    CONDITION QT_FEATURE_abstractbutton AND QT_FEATURE_action
)
qt_feature_definition("toolbutton" "QT_NO_TOOLBUTTON" NEGATE VALUE "1")
qt_feature("toolbar" PUBLIC
    SECTION "Widgets"
    LABEL "QToolBar"
    PURPOSE "Provides movable panels containing a set of controls."
    CONDITION QT_FEATURE_mainwindow
)
qt_feature_definition("toolbar" "QT_NO_TOOLBAR" NEGATE VALUE "1")
qt_feature("toolbox" PUBLIC
    SECTION "Widgets"
    LABEL "QToolBox"
    PURPOSE "Provides columns of tabbed widget items."
    CONDITION QT_FEATURE_toolbutton AND QT_FEATURE_scrollarea
)
qt_feature_definition("toolbox" "QT_NO_TOOLBOX" NEGATE VALUE "1")
qt_feature("groupbox" PUBLIC
    SECTION "Widgets"
    LABEL "QGroupBox"
    PURPOSE "Provides widget grouping boxes with frames."
)
qt_feature_definition("groupbox" "QT_NO_GROUPBOX" NEGATE VALUE "1")
qt_feature("buttongroup" PUBLIC
    SECTION "Widgets"
    LABEL "QButtonGroup"
    PURPOSE "Supports organizing groups of button widgets."
    CONDITION QT_FEATURE_abstractbutton AND QT_FEATURE_groupbox
)
qt_feature_definition("buttongroup" "QT_NO_BUTTONGROUP" NEGATE VALUE "1")
qt_feature("mainwindow" PUBLIC
    SECTION "Widgets"
    LABEL "QMainWindow"
    PURPOSE "Provides main application windows."
    CONDITION QT_FEATURE_menu AND QT_FEATURE_resizehandler AND QT_FEATURE_toolbutton
)
qt_feature_definition("mainwindow" "QT_NO_MAINWINDOW" NEGATE VALUE "1")
qt_feature("dockwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QDockwidget"
    PURPOSE "Supports docking widgets inside a QMainWindow or floated as a top-level window on the desktop."
    CONDITION QT_FEATURE_mainwindow
)
qt_feature_definition("dockwidget" "QT_NO_DOCKWIDGET" NEGATE VALUE "1")
qt_feature("mdiarea" PUBLIC
    SECTION "Widgets"
    LABEL "QMdiArea"
    PURPOSE "Provides an area in which MDI windows are displayed."
    CONDITION QT_FEATURE_scrollarea
)
qt_feature_definition("mdiarea" "QT_NO_MDIAREA" NEGATE VALUE "1")
qt_feature("resizehandler" PUBLIC
    SECTION "Widgets"
    LABEL "QWidgetResizeHandler"
    PURPOSE "Provides an internal resize handler for dock widgets."
)
qt_feature_definition("resizehandler" "QT_NO_RESIZEHANDLER" NEGATE VALUE "1")
qt_feature("statusbar" PUBLIC
    SECTION "Widgets"
    LABEL "QStatusBar"
    PURPOSE "Supports presentation of status information."
)
qt_feature_definition("statusbar" "QT_NO_STATUSBAR" NEGATE VALUE "1")
qt_feature("menubar" PUBLIC
    SECTION "Widgets"
    LABEL "QMenuBar"
    PURPOSE "Provides pull-down menu items."
    CONDITION QT_FEATURE_menu AND QT_FEATURE_toolbutton
)
qt_feature_definition("menubar" "QT_NO_MENUBAR" NEGATE VALUE "1")
qt_feature("contextmenu" PUBLIC
    SECTION "Widgets"
    LABEL "Context menus"
    PURPOSE "Adds pop-up menus on right mouse click to numerous widgets."
    CONDITION QT_FEATURE_menu
)
qt_feature_definition("contextmenu" "QT_NO_CONTEXTMENU" NEGATE VALUE "1")
qt_feature("progressbar" PUBLIC
    SECTION "Widgets"
    LABEL "QProgressBar"
    PURPOSE "Supports presentation of operation progress."
)
qt_feature_definition("progressbar" "QT_NO_PROGRESSBAR" NEGATE VALUE "1")
qt_feature("abstractslider" PUBLIC
    SECTION "Widgets"
    LABEL "QAbstractSlider"
    PURPOSE "Common super class for widgets like QScrollBar, QSlider and QDial."
)
qt_feature("slider" PUBLIC
    SECTION "Widgets"
    LABEL "QSlider"
    PURPOSE "Provides sliders controlling a bounded value."
    CONDITION QT_FEATURE_abstractslider
)
qt_feature_definition("slider" "QT_NO_SLIDER" NEGATE VALUE "1")
qt_feature("scrollbar" PUBLIC
    SECTION "Widgets"
    LABEL "QScrollBar"
    PURPOSE "Provides scrollbars allowing the user access parts of a document that is larger than the widget used to display it."
    CONDITION QT_FEATURE_slider
)
qt_feature_definition("scrollbar" "QT_NO_SCROLLBAR" NEGATE VALUE "1")
qt_feature("dial" PUBLIC
    SECTION "Widgets"
    LABEL "QDial"
    PURPOSE "Provides a rounded range control, e.g., like a speedometer."
    CONDITION QT_FEATURE_slider
)
qt_feature_definition("dial" "QT_NO_DIAL" NEGATE VALUE "1")
qt_feature("scrollarea" PUBLIC
    SECTION "Widgets"
    LABEL "QScrollArea"
    PURPOSE "Supports scrolling views onto widgets."
    CONDITION QT_FEATURE_scrollbar
)
qt_feature_definition("scrollarea" "QT_NO_SCROLLAREA" NEGATE VALUE "1")
qt_feature("scroller" PUBLIC
    SECTION "Widgets"
    LABEL "QScroller"
    PURPOSE "Enables kinetic scrolling for any scrolling widget or graphics item."
    CONDITION QT_FEATURE_easingcurve
)
qt_feature("graphicsview" PUBLIC
    SECTION "Widgets"
    LABEL "QGraphicsView"
    PURPOSE "Provides a canvas/sprite framework."
    CONDITION QT_FEATURE_scrollarea AND QT_FEATURE_widgettextcontrol
)
qt_feature_definition("graphicsview" "QT_NO_GRAPHICSVIEW" NEGATE VALUE "1")
qt_feature("graphicseffect" PUBLIC
    SECTION "Widgets"
    LABEL "QGraphicsEffect"
    PURPOSE "Provides various graphics effects."
    CONDITION QT_FEATURE_graphicsview
)
qt_feature_definition("graphicseffect" "QT_NO_GRAPHICSEFFECT" NEGATE VALUE "1")
qt_feature("textedit" PUBLIC
    SECTION "Widgets"
    LABEL "QTextEdit"
    PURPOSE "Supports rich text editing."
    CONDITION QT_FEATURE_scrollarea AND QT_FEATURE_widgettextcontrol
)
qt_feature_definition("textedit" "QT_NO_TEXTEDIT" NEGATE VALUE "1")
qt_feature("syntaxhighlighter" PUBLIC
    SECTION "Widgets"
    LABEL "QSyntaxHighlighter"
    PURPOSE "Supports custom syntax highlighting."
    CONDITION QT_FEATURE_textedit
)
qt_feature_definition("syntaxhighlighter" "QT_NO_SYNTAXHIGHLIGHTER" NEGATE VALUE "1")
qt_feature("rubberband" PUBLIC
    SECTION "Widgets"
    LABEL "QRubberBand"
    PURPOSE "Supports using rubberbands to indicate selections and boundaries."
)
qt_feature_definition("rubberband" "QT_NO_RUBBERBAND" NEGATE VALUE "1")
qt_feature("tooltip" PUBLIC
    SECTION "Widgets"
    LABEL "QToolTip"
    PURPOSE "Supports presentation of tooltips."
    CONDITION QT_FEATURE_label
)
qt_feature_definition("tooltip" "QT_NO_TOOLTIP" NEGATE VALUE "1")
qt_feature("statustip" PUBLIC
    SECTION "Widgets"
    LABEL "Status Tip"
    PURPOSE "Supports status tip functionality and events."
)
qt_feature_definition("statustip" "QT_NO_STATUSTIP" NEGATE VALUE "1")
qt_feature("sizegrip" PUBLIC
    SECTION "Widgets"
    LABEL "QSizeGrip"
    PURPOSE "Provides corner-grips for resizing top-level windows."
)
qt_feature_definition("sizegrip" "QT_NO_SIZEGRIP" NEGATE VALUE "1")
qt_feature("calendarwidget" PUBLIC
    SECTION "Widgets"
    LABEL "QCalendarWidget"
    PURPOSE "Provides a monthly based calendar widget allowing the user to select a date."
    CONDITION ( QT_FEATURE_label ) AND ( QT_FEATURE_menu ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_spinbox ) AND ( QT_FEATURE_tableview ) AND ( QT_FEATURE_textdate ) AND ( QT_FEATURE_toolbutton )
)
qt_feature_definition("calendarwidget" "QT_NO_CALENDARWIDGET" NEGATE VALUE "1")
qt_feature("keysequenceedit" PUBLIC
    SECTION "Widgets"
    LABEL "QKeySequenceEdit"
    PURPOSE "Provides a widget for editing QKeySequences."
    CONDITION QT_FEATURE_lineedit AND QT_FEATURE_shortcut
)
qt_feature_definition("keysequenceedit" "QT_NO_KEYSEQUENCEEDIT" NEGATE VALUE "1")
qt_feature("dialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QDialog"
    PURPOSE "Base class of dialog windows."
)
qt_feature("dialogbuttonbox" PUBLIC
    SECTION "Dialogs"
    LABEL "QDialogButtonBox"
    PURPOSE "Presents buttons in a layout that is appropriate for the current widget style."
    CONDITION QT_FEATURE_dialog AND QT_FEATURE_pushbutton
)
qt_feature("messagebox" PUBLIC
    SECTION "Dialogs"
    LABEL "QMessageBox"
    PURPOSE "Provides message boxes displaying informative messages and simple questions."
    CONDITION ( QT_FEATURE_checkbox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton )
)
qt_feature_definition("messagebox" "QT_NO_MESSAGEBOX" NEGATE VALUE "1")
qt_feature("colordialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QColorDialog"
    PURPOSE "Provides a dialog widget for specifying colors."
    CONDITION ( QT_FEATURE_dialog ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_spinbox )
)
qt_feature_definition("colordialog" "QT_NO_COLORDIALOG" NEGATE VALUE "1")
qt_feature("filedialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QFileDialog"
    PURPOSE "Provides a dialog widget for selecting files or directories."
    CONDITION ( QT_FEATURE_buttongroup ) AND ( QT_FEATURE_combobox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_filesystemmodel ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_proxymodel ) AND ( QT_FEATURE_splitter ) AND ( QT_FEATURE_stackedwidget ) AND ( QT_FEATURE_treeview ) AND ( QT_FEATURE_toolbutton )
)
qt_feature_definition("filedialog" "QT_NO_FILEDIALOG" NEGATE VALUE "1")
qt_feature("fontdialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QFontDialog"
    PURPOSE "Provides a dialog widget for selecting fonts."
    CONDITION ( QT_FEATURE_checkbox ) AND ( QT_FEATURE_combobox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_groupbox ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_stringlistmodel ) AND ( QT_FEATURE_validator )
)
qt_feature_definition("fontdialog" "QT_NO_FONTDIALOG" NEGATE VALUE "1")
qt_feature("progressdialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QProgressDialog"
    PURPOSE "Provides feedback on the progress of a slow operation."
    CONDITION ( QT_FEATURE_dialog ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_progressbar )
)
qt_feature_definition("progressdialog" "QT_NO_PROGRESSDIALOG" NEGATE VALUE "1")
qt_feature("inputdialog" PUBLIC
    SECTION "Dialogs"
    LABEL "QInputDialog"
    PURPOSE "Provides a simple convenience dialog to get a single value from the user."
    CONDITION ( QT_FEATURE_combobox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_dialogbuttonbox ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_spinbox ) AND ( QT_FEATURE_stackedwidget ) AND ( QT_FEATURE_textedit )
)
qt_feature_definition("inputdialog" "QT_NO_INPUTDIALOG" NEGATE VALUE "1")
qt_feature("errormessage" PUBLIC
    SECTION "Dialogs"
    LABEL "QErrorMessage"
    PURPOSE "Provides an error message display dialog."
    CONDITION ( QT_FEATURE_checkbox ) AND ( QT_FEATURE_dialog ) AND ( QT_FEATURE_textedit ) AND ( QT_FEATURE_label ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_textedit )
)
qt_feature_definition("errormessage" "QT_NO_ERRORMESSAGE" NEGATE VALUE "1")
qt_feature("wizard" PUBLIC
    SECTION "Dialogs"
    LABEL "QWizard"
    PURPOSE "Provides a framework for multi-page click-through dialogs."
    CONDITION ( QT_FEATURE_dialog ) AND ( QT_FEATURE_pushbutton ) AND ( QT_FEATURE_label )
)
qt_feature_definition("wizard" "QT_NO_WIZARD" NEGATE VALUE "1")
qt_feature("listview" PUBLIC
    SECTION "ItemViews"
    LABEL "QListView"
    PURPOSE "Provides a list or icon view onto a model."
    CONDITION QT_FEATURE_itemviews
)
qt_feature_definition("listview" "QT_NO_LISTVIEW" NEGATE VALUE "1")
qt_feature("tableview" PUBLIC
    SECTION "ItemViews"
    LABEL "QTableView"
    PURPOSE "Provides a default model/view implementation of a table view."
    CONDITION QT_FEATURE_itemviews
)
qt_feature_definition("tableview" "QT_NO_TABLEVIEW" NEGATE VALUE "1")
qt_feature("treeview" PUBLIC
    SECTION "ItemViews"
    LABEL "QTreeView"
    PURPOSE "Provides a default model/view implementation of a tree view."
    CONDITION QT_FEATURE_itemviews
)
qt_feature_definition("treeview" "QT_NO_TREEVIEW" NEGATE VALUE "1")
qt_feature("datawidgetmapper" PUBLIC
    SECTION "ItemViews"
    LABEL "QDataWidgetMapper"
    PURPOSE "Provides mapping between a section of a data model to widgets."
    CONDITION QT_FEATURE_itemviews
)
qt_feature_definition("datawidgetmapper" "QT_NO_DATAWIDGETMAPPER" NEGATE VALUE "1")
qt_feature("columnview" PUBLIC
    SECTION "ItemViews"
    LABEL "QColumnView"
    PURPOSE "Provides a model/view implementation of a column view."
    CONDITION QT_FEATURE_listview
)
qt_feature_definition("columnview" "QT_NO_COLUMNVIEW" NEGATE VALUE "1")
qt_feature("completer" PUBLIC
    SECTION "Utilities"
    LABEL "QCompleter"
    PURPOSE "Provides completions based on an item model."
    CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_itemviews
)
qt_feature_definition("completer" "QT_NO_COMPLETER" NEGATE VALUE "1")
qt_feature("fscompleter" PUBLIC
    SECTION "Utilities"
    LABEL "QFSCompleter"
    PURPOSE "Provides file name completion in QFileDialog."
    CONDITION QT_FEATURE_filesystemmodel AND QT_FEATURE_completer
)
qt_feature_definition("fscompleter" "QT_NO_FSCOMPLETER" NEGATE VALUE "1")
qt_feature("undoview" PUBLIC
    SECTION "Utilities"
    LABEL "QUndoView"
    PURPOSE "Provides a widget which shows the contents of an undo stack."
    CONDITION QT_FEATURE_undostack AND QT_FEATURE_listview
)
qt_feature_definition("undoview" "QT_NO_UNDOVIEW" NEGATE VALUE "1")
qt_configure_add_summary_section(NAME "Qt Widgets")
qt_configure_add_summary_entry(ARGS "gtk3")
qt_configure_add_summary_entry(
    TYPE "featureList"
    ARGS "style-fusion style-mac style-windows style-windowsvista style-android"
    MESSAGE "Styles"
)
qt_configure_end_summary_section() # end of "Qt Widgets" section
