/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
 * All features and their dependencies.
 *
 * This list is generated from $QTDIR/src/corelib/global/qfeatures.txt
 * by $QTSRCDIR/util/scripts/make_qfeatures_dot_h
 */

// QAction
//#define QT_NO_ACTION

// QClipboard
//#define QT_NO_CLIPBOARD

// Color Names
//#define QT_NO_COLORNAMES

// QtConcurrent
//#define QT_NO_CONCURRENT

// CssParser
//#define QT_NO_CSSPARSER

// QCursor
//#define QT_NO_CURSOR

// QDesktopServices
//#define QT_NO_DESKTOPSERVICES

// Document Object Model
//#define QT_NO_DOM

// Effects
//#define QT_NO_EFFECTS

// QFileSystemIterator
//#define QT_NO_FILESYSTEMITERATOR

// QFileSystemWatcher
//#define QT_NO_FILESYSTEMWATCHER

// Freetype Font Engine
//#define QT_NO_FREETYPE

// Gesture
//#define QT_NO_GESTURES

// QGroupBox
//#define QT_NO_GROUPBOX

// QHostInfo
//#define QT_NO_HOSTINFO

// BMP Image Format
//#define QT_NO_IMAGEFORMAT_BMP

// JPEG Image Format
//#define QT_NO_IMAGEFORMAT_JPEG

// PNG Image Format
//#define QT_NO_IMAGEFORMAT_PNG

// PPM Image Format
//#define QT_NO_IMAGEFORMAT_PPM

// XBM Image Format
//#define QT_NO_IMAGEFORMAT_XBM

// XPM Image Format
//#define QT_NO_IMAGEFORMAT_XPM

// QImage::createHeuristicMask()
//#define QT_NO_IMAGE_HEURISTIC_MASK

// Image Text
//#define QT_NO_IMAGE_TEXT

// QLCDNumber
//#define QT_NO_LCDNUMBER

// QLibrary
//#define QT_NO_LIBRARY

// QLineEdit
//#define QT_NO_LINEEDIT

// QMessageBox
//#define QT_NO_MESSAGEBOX

// QMovie
//#define QT_NO_MOVIE

// QNetworkInterface
//#define QT_NO_NETWORKINTERFACE

// QNetworkProxy
//#define QT_NO_NETWORKPROXY

// Qt::WA_PaintOnScreen
//#define QT_NO_PAINTONSCREEN

// Painting Debug Utilities
//#define QT_NO_PAINT_DEBUG

// QPicture
//#define QT_NO_PICTURE

// QProcess
//#define QT_NO_PROCESS

// QProgressBar
//#define QT_NO_PROGRESSBAR

// Properties
//#define QT_NO_PROPERTIES

// QRegularExpression
//#define QT_NO_REGULAREXPRESSION

// Resize Handler
//#define QT_NO_RESIZEHANDLER

// QRubberBand
//#define QT_NO_RUBBERBAND

// Session Manager
//#define QT_NO_SESSIONMANAGER

// QSettings
//#define QT_NO_SETTINGS

// QSharedMemory
//#define QT_NO_SHAREDMEMORY

// QShortcut
//#define QT_NO_SHORTCUT

// QSizeGrip
//#define QT_NO_SIZEGRIP

// QSlider
//#define QT_NO_SLIDER

// Spin Widget
//#define QT_NO_SPINWIDGET

// Splash screen widget
//#define QT_NO_SPLASHSCREEN

// QStackedWidget
//#define QT_NO_STACKEDWIDGET

// QStatusBar
//#define QT_NO_STATUSBAR

// Status Tip
//#define QT_NO_STATUSTIP

// QWindowsStyle
//#define QT_NO_STYLE_WINDOWS

// QSystemSemaphore
//#define QT_NO_SYSTEMSEMAPHORE

// QSystemTrayIcon
//#define QT_NO_SYSTEMTRAYICON

// QTabletEvent
//#define QT_NO_TABLETEVENT

// QTemporaryFile
//#define QT_NO_TEMPORARYFILE

// QTextCodec
//#define QT_NO_TEXTCODEC

// Text Date
//#define QT_NO_TEXTDATE

// HtmlParser
//#define QT_NO_TEXTHTMLPARSER

// QToolTip
//#define QT_NO_TOOLTIP

// Translation
//#define QT_NO_TRANSLATION

// QUdpSocket
//#define QT_NO_UDPSOCKET

// QUndoCommand
//#define QT_NO_UNDOCOMMAND

// QValidator
//#define QT_NO_VALIDATOR

// QWheelEvent
//#define QT_NO_WHEELEVENT

// 
//#define QT_NO_XMLSTREAM

// Animation
#if !defined(QT_NO_ANIMATION) && (defined(QT_NO_PROPERTIES))
#define QT_NO_ANIMATION
#endif

// Big Codecs
#if !defined(QT_NO_BIG_CODECS) && (defined(QT_NO_TEXTCODEC))
#define QT_NO_BIG_CODECS
#endif

// QButtonGroup
#if !defined(QT_NO_BUTTONGROUP) && (defined(QT_NO_GROUPBOX))
#define QT_NO_BUTTONGROUP
#endif

// Codecs
#if !defined(QT_NO_CODECS) && (defined(QT_NO_TEXTCODEC))
#define QT_NO_CODECS
#endif

// QDate/QTime/QDateTime
#if !defined(QT_NO_DATESTRING) && (defined(QT_NO_TEXTDATE))
#define QT_NO_DATESTRING
#endif

// QDial
#if !defined(QT_NO_DIAL) && (defined(QT_NO_SLIDER))
#define QT_NO_DIAL
#endif

// Drag and drop
#if !defined(QT_NO_DRAGANDDROP) && (defined(QT_NO_IMAGEFORMAT_XPM))
#define QT_NO_DRAGANDDROP
#endif

// QFileSystemModel
#if !defined(QT_NO_FILESYSTEMMODEL) && (defined(QT_NO_FILESYSTEMWATCHER))
#define QT_NO_FILESYSTEMMODEL
#endif

// File Transfer Protocol
#if !defined(QT_NO_FTP) && (defined(QT_NO_TEXTDATE))
#define QT_NO_FTP
#endif

// Hyper Text Transfer Protocol
#if !defined(QT_NO_HTTP) && (defined(QT_NO_HOSTINFO))
#define QT_NO_HTTP
#endif

// iconv
#if !defined(QT_NO_ICONV) && (defined(QT_NO_TEXTCODEC))
#define QT_NO_ICONV
#endif

// QInputContext
#if !defined(QT_NO_IM) && (defined(QT_NO_LIBRARY))
#define QT_NO_IM
#endif

// QImageIOPlugin
#if !defined(QT_NO_IMAGEFORMATPLUGIN) && (defined(QT_NO_LIBRARY))
#define QT_NO_IMAGEFORMATPLUGIN
#endif

// QLocalServer
#if !defined(QT_NO_LOCALSERVER) && (defined(QT_NO_TEMPORARYFILE))
#define QT_NO_LOCALSERVER
#endif

// QPdf
#if !defined(QT_NO_PDF) && (defined(QT_NO_TEMPORARYFILE))
#define QT_NO_PDF
#endif

// QMenu
#if !defined(QT_NO_MENU) && (defined(QT_NO_ACTION))
#define QT_NO_MENU
#endif

// QNetworkDiskCache
#if !defined(QT_NO_NETWORKDISKCACHE) && (defined(QT_NO_TEMPORARYFILE))
#define QT_NO_NETWORKDISKCACHE
#endif

// QProgressDialog
#if !defined(QT_NO_PROGRESSDIALOG) && (defined(QT_NO_PROGRESSBAR))
#define QT_NO_PROGRESSDIALOG
#endif

// QScrollBar
#if !defined(QT_NO_SCROLLBAR) && (defined(QT_NO_SLIDER))
#define QT_NO_SCROLLBAR
#endif

//  SOCKS5
#if !defined(QT_NO_SOCKS5) && (defined(QT_NO_NETWORKPROXY))
#define QT_NO_SOCKS5
#endif

// QSplitter
#if !defined(QT_NO_SPLITTER) && (defined(QT_NO_RUBBERBAND))
#define QT_NO_SPLITTER
#endif

// State machine
#if !defined(QT_NO_STATEMACHINE) && (defined(QT_NO_PROPERTIES))
#define QT_NO_STATEMACHINE
#endif

// QFusionStyle
#if !defined(QT_NO_STYLE_FUSION) && (defined(QT_NO_IMAGEFORMAT_XPM))
#define QT_NO_STYLE_FUSION
#endif

// QWindowsXPStyle
#if !defined(QT_NO_STYLE_WINDOWSXP) && (defined(QT_NO_STYLE_WINDOWS))
#define QT_NO_STYLE_WINDOWSXP
#endif

// QToolButton
#if !defined(QT_NO_TOOLBUTTON) && (defined(QT_NO_ACTION))
#define QT_NO_TOOLBUTTON
#endif

// QUndoStack
#if !defined(QT_NO_UNDOSTACK) && (defined(QT_NO_UNDOCOMMAND))
#define QT_NO_UNDOSTACK
#endif

// QWizard
#if !defined(QT_NO_WIZARD) && (defined(QT_NO_PROPERTIES))
#define QT_NO_WIZARD
#endif

// QXmlStreamReader
#if !defined(QT_NO_XMLSTREAMREADER) && (defined(QT_NO_XMLSTREAM))
#define QT_NO_XMLSTREAMREADER
#endif

// QXmlStreamWriter
#if !defined(QT_NO_XMLSTREAMWRITER) && (defined(QT_NO_XMLSTREAM))
#define QT_NO_XMLSTREAMWRITER
#endif

// Context menu
#if !defined(QT_NO_CONTEXTMENU) && (defined(QT_NO_MENU))
#define QT_NO_CONTEXTMENU
#endif

// QPrinter
#if !defined(QT_NO_PRINTER) && (defined(QT_NO_PICTURE) || defined(QT_NO_TEMPORARYFILE))
#define QT_NO_PRINTER
#endif

// QScrollArea
#if !defined(QT_NO_SCROLLAREA) && (defined(QT_NO_SCROLLBAR))
#define QT_NO_SCROLLAREA
#endif

// QWindowsCEStyle
#if !defined(QT_NO_STYLE_WINDOWSCE) && (defined(QT_NO_STYLE_WINDOWS) || defined(QT_NO_IMAGEFORMAT_XPM))
#define QT_NO_STYLE_WINDOWSCE
#endif

// QWindowsMobileStyle
#if !defined(QT_NO_STYLE_WINDOWSMOBILE) && (defined(QT_NO_STYLE_WINDOWS) || defined(QT_NO_IMAGEFORMAT_XPM))
#define QT_NO_STYLE_WINDOWSMOBILE
#endif

// QWindowsVistaStyle
#if !defined(QT_NO_STYLE_WINDOWSVISTA) && (defined(QT_NO_STYLE_WINDOWSXP))
#define QT_NO_STYLE_WINDOWSVISTA
#endif

// QTabBar
#if !defined(QT_NO_TABBAR) && (defined(QT_NO_TOOLBUTTON))
#define QT_NO_TABBAR
#endif

// OdfWriter
#if !defined(QT_NO_TEXTODFWRITER) && (defined(QT_NO_XMLSTREAMWRITER))
#define QT_NO_TEXTODFWRITER
#endif

// Translation (UTF-8 representation)
#if !defined(QT_NO_TRANSLATION_UTF8) && (defined(QT_NO_TRANSLATION) || defined(QT_NO_TEXTCODEC))
#define QT_NO_TRANSLATION_UTF8
#endif

// QUndoGroup
#if !defined(QT_NO_UNDOGROUP) && (defined(QT_NO_UNDOSTACK))
#define QT_NO_UNDOGROUP
#endif

// QWhatsThis
#if !defined(QT_NO_WHATSTHIS) && (defined(QT_NO_TOOLBUTTON))
#define QT_NO_WHATSTHIS
#endif

// Bearer Management
#if !defined(QT_NO_BEARERMANAGEMENT) && (defined(QT_NO_LIBRARY) || defined(QT_NO_NETWORKINTERFACE) || defined(QT_NO_PROPERTIES))
#define QT_NO_BEARERMANAGEMENT
#endif

// Qt D-Bus module
#if !defined(QT_NO_DBUS) && (defined(QT_NO_PROPERTIES) || defined(QT_NO_XMLSTREAMREADER))
#define QT_NO_DBUS
#endif

// QGraphicsView
#if !defined(QT_NO_GRAPHICSVIEW) && (defined(QT_NO_SCROLLAREA))
#define QT_NO_GRAPHICSVIEW
#endif

// QMdiArea
#if !defined(QT_NO_MDIAREA) && (defined(QT_NO_SCROLLAREA))
#define QT_NO_MDIAREA
#endif

// QSpinBox
#if !defined(QT_NO_SPINBOX) && (defined(QT_NO_SPINWIDGET) || defined(QT_NO_LINEEDIT) || defined(QT_NO_VALIDATOR))
#define QT_NO_SPINBOX
#endif

// QStyleSheetStyle
#if !defined(QT_NO_STYLE_STYLESHEET) && (defined(QT_NO_STYLE_WINDOWS) || defined(QT_NO_PROPERTIES) || defined(QT_NO_CSSPARSER))
#define QT_NO_STYLE_STYLESHEET
#endif

// QColorDialog
#if !defined(QT_NO_COLORDIALOG) && (defined(QT_NO_SPINBOX))
#define QT_NO_COLORDIALOG
#endif

// Common UNIX Printing System
#if !defined(QT_NO_CUPS) && (defined(QT_NO_PRINTER) || defined(QT_NO_LIBRARY))
#define QT_NO_CUPS
#endif

// QGraphicsEffect
#if !defined(QT_NO_GRAPHICSEFFECT) && (defined(QT_NO_GRAPHICSVIEW))
#define QT_NO_GRAPHICSEFFECT
#endif

// The Model/View Framework
#if !defined(QT_NO_ITEMVIEWS) && (defined(QT_NO_RUBBERBAND) || defined(QT_NO_SCROLLAREA))
#define QT_NO_ITEMVIEWS
#endif

// QMenuBar
#if !defined(QT_NO_MENUBAR) && (defined(QT_NO_MENU) || defined(QT_NO_TOOLBUTTON))
#define QT_NO_MENUBAR
#endif

// QTabWidget
#if !defined(QT_NO_TABWIDGET) && (defined(QT_NO_TABBAR) || defined(QT_NO_STACKEDWIDGET))
#define QT_NO_TABWIDGET
#endif

// QTextEdit
#if !defined(QT_NO_TEXTEDIT) && (defined(QT_NO_SCROLLAREA) || defined(QT_NO_PROPERTIES))
#define QT_NO_TEXTEDIT
#endif

// QErrorMessage
#if !defined(QT_NO_ERRORMESSAGE) && (defined(QT_NO_TEXTEDIT))
#define QT_NO_ERRORMESSAGE
#endif

// QListView
#if !defined(QT_NO_LISTVIEW) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_LISTVIEW
#endif

// QMainWindow
#if !defined(QT_NO_MAINWINDOW) && (defined(QT_NO_MENU) || defined(QT_NO_RESIZEHANDLER) || defined(QT_NO_TOOLBUTTON))
#define QT_NO_MAINWINDOW
#endif

// QAbstractProxyModel
#if !defined(QT_NO_PROXYMODEL) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_PROXYMODEL
#endif

// QStandardItemModel
#if !defined(QT_NO_STANDARDITEMMODEL) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_STANDARDITEMMODEL
#endif

// QStringListModel
#if !defined(QT_NO_STRINGLISTMODEL) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_STRINGLISTMODEL
#endif

// QSyntaxHighlighter
#if !defined(QT_NO_SYNTAXHIGHLIGHTER) && (defined(QT_NO_TEXTEDIT))
#define QT_NO_SYNTAXHIGHLIGHTER
#endif

// QTableView
#if !defined(QT_NO_TABLEVIEW) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_TABLEVIEW
#endif

// QTextBrowser
#if !defined(QT_NO_TEXTBROWSER) && (defined(QT_NO_TEXTEDIT))
#define QT_NO_TEXTBROWSER
#endif

// QToolBox
#if !defined(QT_NO_TOOLBOX) && (defined(QT_NO_TOOLBUTTON) || defined(QT_NO_SCROLLAREA))
#define QT_NO_TOOLBOX
#endif

// QTreeView
#if !defined(QT_NO_TREEVIEW) && (defined(QT_NO_ITEMVIEWS))
#define QT_NO_TREEVIEW
#endif

// Accessibility
#if !defined(QT_NO_ACCESSIBILITY) && (defined(QT_NO_PROPERTIES) || defined(QT_NO_MENUBAR))
#define QT_NO_ACCESSIBILITY
#endif

// QColumnView
#if !defined(QT_NO_COLUMNVIEW) && (defined(QT_NO_LISTVIEW))
#define QT_NO_COLUMNVIEW
#endif

// QCompleter
#if !defined(QT_NO_COMPLETER) && (defined(QT_NO_PROXYMODEL))
#define QT_NO_COMPLETER
#endif

// QDataWidgetMapper
#if !defined(QT_NO_DATAWIDGETMAPPER) && (defined(QT_NO_ITEMVIEWS) || defined(QT_NO_PROPERTIES))
#define QT_NO_DATAWIDGETMAPPER
#endif

// QIdentityProxyModel
#if !defined(QT_NO_IDENTITYPROXYMODEL) && (defined(QT_NO_PROXYMODEL))
#define QT_NO_IDENTITYPROXYMODEL
#endif

// QListWidget
#if !defined(QT_NO_LISTWIDGET) && (defined(QT_NO_LISTVIEW))
#define QT_NO_LISTWIDGET
#endif

// QSortFilterProxyModel
#if !defined(QT_NO_SORTFILTERPROXYMODEL) && (defined(QT_NO_PROXYMODEL))
#define QT_NO_SORTFILTERPROXYMODEL
#endif

// QTableWidget
#if !defined(QT_NO_TABLEWIDGET) && (defined(QT_NO_TABLEVIEW))
#define QT_NO_TABLEWIDGET
#endif

// QToolBar
#if !defined(QT_NO_TOOLBAR) && (defined(QT_NO_MAINWINDOW))
#define QT_NO_TOOLBAR
#endif

// QTreeWidget
#if !defined(QT_NO_TREEWIDGET) && (defined(QT_NO_TREEVIEW))
#define QT_NO_TREEWIDGET
#endif

// QDirModel
#if !defined(QT_NO_DIRMODEL) && (defined(QT_NO_ITEMVIEWS) || defined(QT_NO_FILESYSTEMMODEL))
#define QT_NO_DIRMODEL
#endif

// QDockwidget
#if !defined(QT_NO_DOCKWIDGET) && (defined(QT_NO_RUBBERBAND) || defined(QT_NO_MAINWINDOW))
#define QT_NO_DOCKWIDGET
#endif

// QUndoView
#if !defined(QT_NO_UNDOVIEW) && (defined(QT_NO_UNDOSTACK) || defined(QT_NO_LISTVIEW))
#define QT_NO_UNDOVIEW
#endif

// QCompleter
#if !defined(QT_NO_FSCOMPLETER) && (defined(QT_NO_FILESYSTEMMODEL) || defined(QT_NO_COMPLETER))
#define QT_NO_FSCOMPLETER
#endif

// QComboBox
#if !defined(QT_NO_COMBOBOX) && (defined(QT_NO_LINEEDIT) || defined(QT_NO_STANDARDITEMMODEL) || defined(QT_NO_LISTVIEW))
#define QT_NO_COMBOBOX
#endif

// QPrintPreviewWidget
#if !defined(QT_NO_PRINTPREVIEWWIDGET) && (defined(QT_NO_GRAPHICSVIEW) || defined(QT_NO_PRINTER) || defined(QT_NO_MAINWINDOW))
#define QT_NO_PRINTPREVIEWWIDGET
#endif

// QCalendarWidget
#if !defined(QT_NO_CALENDARWIDGET) && (defined(QT_NO_TABLEVIEW) || defined(QT_NO_MENU) || defined(QT_NO_TEXTDATE) || defined(QT_NO_SPINBOX) || defined(QT_NO_TOOLBUTTON))
#define QT_NO_CALENDARWIDGET
#endif

// QDateTimeEdit
#if !defined(QT_NO_DATETIMEEDIT) && (defined(QT_NO_CALENDARWIDGET) || defined(QT_NO_DATESTRING))
#define QT_NO_DATETIMEEDIT
#endif

// QInputDialog
#if !defined(QT_NO_INPUTDIALOG) && (defined(QT_NO_COMBOBOX) || defined(QT_NO_SPINBOX) || defined(QT_NO_STACKEDWIDGET))
#define QT_NO_INPUTDIALOG
#endif

// QFontComboBox
#if !defined(QT_NO_FONTCOMBOBOX) && (defined(QT_NO_COMBOBOX) || defined(QT_NO_STRINGLISTMODEL))
#define QT_NO_FONTCOMBOBOX
#endif

// QFontDialog
#if !defined(QT_NO_FONTDIALOG) && (defined(QT_NO_STRINGLISTMODEL) || defined(QT_NO_COMBOBOX) || defined(QT_NO_VALIDATOR) || defined(QT_NO_GROUPBOX))
#define QT_NO_FONTDIALOG
#endif

// QPrintDialog
#if !defined(QT_NO_PRINTDIALOG) && (defined(QT_NO_PRINTER) || defined(QT_NO_COMBOBOX) || defined(QT_NO_BUTTONGROUP) || defined(QT_NO_SPINBOX) || defined(QT_NO_TREEVIEW) || defined(QT_NO_TABWIDGET))
#define QT_NO_PRINTDIALOG
#endif

// QFileDialog
#if !defined(QT_NO_FILEDIALOG) && (defined(QT_NO_DIRMODEL) || defined(QT_NO_TREEVIEW) || defined(QT_NO_COMBOBOX) || defined(QT_NO_TOOLBUTTON) || defined(QT_NO_BUTTONGROUP) || defined(QT_NO_TOOLTIP) || defined(QT_NO_SPLITTER) || defined(QT_NO_STACKEDWIDGET) || defined(QT_NO_PROXYMODEL))
#define QT_NO_FILEDIALOG
#endif

// QPrintPreviewDialog
#if !defined(QT_NO_PRINTPREVIEWDIALOG) && (defined(QT_NO_PRINTPREVIEWWIDGET) || defined(QT_NO_PRINTDIALOG) || defined(QT_NO_TOOLBAR))
#define QT_NO_PRINTPREVIEWDIALOG
#endif

