/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qabstracteventdispatcher.h"
#include "qapplication.h"
#include "qclipboard.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qdir.h"
#include "qevent.h"
#include "qfile.h"
#include "qfileinfo.h"
#if QT_CONFIG(graphicsview)
#include "qgraphicsscene.h"
#include <QtWidgets/qgraphicsproxywidget.h>
#endif
#include "qhash.h"
#include "qset.h"
#include "qlayout.h"
#include "qpixmapcache.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylefactory.h"
#include "qtooltip.h"
#include "qtranslator.h"
#include "qvariant.h"
#include "qwidget.h"
#if QT_CONFIG(draganddrop)
#include <private/qdnd_p.h>
#endif
#include "private/qguiapplication_p.h"
#include "qcolormap.h"
#include "qdebug.h"
#include "private/qstylesheetstyle_p.h"
#include "private/qstyle_p.h"
#if QT_CONFIG(messagebox)
#include "qmessagebox.h"
#endif
#include "qwidgetwindow_p.h"
#include <QtGui/qstylehints.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/qtouchdevice.h>
#include <qpa/qplatformtheme.h>
#if QT_CONFIG(whatsthis)
#include <QtWidgets/QWhatsThis>
#endif

#include "private/qkeymapper_p.h"
#include "private/qaccessiblewidgetfactory_p.h"

#include <qthread.h>
#include <private/qthread_p.h>

#include <private/qfont_p.h>

#include <stdlib.h>

#include "qapplication_p.h"
#include "private/qevent_p.h"
#include "qwidget_p.h"

#include "qgesture.h"
#include "private/qgesturemanager_p.h"
#include <qpa/qplatformfontdatabase.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h> // for qt_win_display_dc()
#endif

#include "qdatetime.h"

#include <qpa/qplatformwindow.h>

#include <qtwidgets_tracepoints_p.h>

#include <algorithm>
#include <iterator>

//#define ALIEN_DEBUG

static void initResources()
{
    Q_INIT_RESOURCE(qstyle);

#if QT_CONFIG(messagebox)
    Q_INIT_RESOURCE(qmessagebox);
#endif
}

QT_BEGIN_NAMESPACE

// Helper macro for static functions to check on the existence of the application class.
#define CHECK_QAPP_INSTANCE(...) \
    if (Q_LIKELY(QCoreApplication::instance())) { \
    } else { \
        qWarning("Must construct a QApplication first."); \
        return __VA_ARGS__; \
    }

Q_CORE_EXPORT void qt_call_post_routines();
Q_GUI_EXPORT bool qt_sendShortcutOverrideEvent(QObject *o, ulong timestamp, int k, Qt::KeyboardModifiers mods, const QString &text = QString(), bool autorep = false, ushort count = 1);

QApplicationPrivate *QApplicationPrivate::self = nullptr;

bool QApplicationPrivate::autoSipEnabled = true;

QApplicationPrivate::QApplicationPrivate(int &argc, char **argv, int flags)
    : QApplicationPrivateBase(argc, argv, flags)
{
    application_type = QApplicationPrivate::Gui;

#ifndef QT_NO_GESTURES
    gestureManager = nullptr;
    gestureWidget = nullptr;
#endif // QT_NO_GESTURES

    if (!self)
        self = this;
}

QApplicationPrivate::~QApplicationPrivate()
{
    if (self == this)
        self = nullptr;
}

void QApplicationPrivate::createEventDispatcher()
{
    QGuiApplicationPrivate::createEventDispatcher();
}

/*!
    \class QApplication
    \brief The QApplication class manages the GUI application's control
    flow and main settings.

    \inmodule QtWidgets

    QApplication specializes QGuiApplication with some functionality needed
    for QWidget-based applications. It handles widget specific initialization,
    finalization.

    For any GUI application using Qt, there is precisely \b one QApplication
    object, no matter whether the application has 0, 1, 2 or more windows at
    any given time. For non-QWidget based Qt applications, use QGuiApplication instead,
    as it does not depend on the \l QtWidgets library.

    Some GUI applications provide a special batch mode ie. provide command line
    arguments for executing tasks without manual intervention. In such non-GUI
    mode, it is often sufficient to instantiate a plain QCoreApplication to
    avoid unnecessarily initializing resources needed for a graphical user
    interface. The following example shows how to dynamically create an
    appropriate type of application instance:

    \snippet code/src_gui_kernel_qapplication.cpp 0

    The QApplication object is accessible through the instance() function that
    returns a pointer equivalent to the global qApp pointer.

    QApplication's main areas of responsibility are:
        \list
            \li  It initializes the application with the user's desktop settings
                such as palette(), font() and doubleClickInterval(). It keeps
                track of these properties in case the user changes the desktop
                globally, for example through some kind of control panel.

            \li  It performs event handling, meaning that it receives events
                from the underlying window system and dispatches them to the
                relevant widgets. By using sendEvent() and postEvent() you can
                send your own events to widgets.

            \li  It parses common command line arguments and sets its internal
                state accordingly. See the \l{QApplication::QApplication()}
                {constructor documentation} below for more details.

            \li  It defines the application's look and feel, which is
                encapsulated in a QStyle object. This can be changed at runtime
                with setStyle().

            \li  It provides localization of strings that are visible to the
                user via translate().

            \li  It provides some magical objects like the desktop() and the
                clipboard().

            \li  It knows about the application's windows. You can ask which
                widget is at a certain position using widgetAt(), get a list of
                topLevelWidgets() and closeAllWindows(), etc.

            \li  It manages the application's mouse cursor handling, see
                setOverrideCursor()
        \endlist

    Since the QApplication object does so much initialization, it \e{must} be
    created before any other objects related to the user interface are created.
    QApplication also deals with common command line arguments. Hence, it is
    usually a good idea to create it \e before any interpretation or
    modification of \c argv is done in the application itself.

    \table
    \header
        \li{2,1} Groups of functions

        \row
        \li  System settings
        \li  desktopSettingsAware(),
            setDesktopSettingsAware(),
            cursorFlashTime(),
            setCursorFlashTime(),
            doubleClickInterval(),
            setDoubleClickInterval(),
            setKeyboardInputInterval(),
            wheelScrollLines(),
            setWheelScrollLines(),
            palette(),
            setPalette(),
            font(),
            setFont(),
            fontMetrics().

        \row
        \li  Event handling
        \li  exec(),
            processEvents(),
            exit(),
            quit().
            sendEvent(),
            postEvent(),
            sendPostedEvents(),
            removePostedEvents(),
            hasPendingEvents(),
            notify().

        \row
        \li  GUI Styles
        \li  style(),
            setStyle().

        \row
        \li  Text handling
        \li  installTranslator(),
            removeTranslator()
            translate().

        \row
        \li  Widgets
        \li  allWidgets(),
            topLevelWidgets(),
            desktop(),
            activePopupWidget(),
            activeModalWidget(),
            clipboard(),
            focusWidget(),
            activeWindow(),
            widgetAt().

        \row
        \li  Advanced cursor handling
        \li  overrideCursor(),
            setOverrideCursor(),
            restoreOverrideCursor().

        \row
        \li  Miscellaneous
        \li  closeAllWindows(),
            startingUp(),
            closingDown().
    \endtable

    \sa QCoreApplication, QAbstractEventDispatcher, QEventLoop, QSettings
*/

#if QT_DEPRECATED_SINCE(5, 8)
// ### fixme: Qt 6: Remove ColorSpec and accessors.
/*!
    \enum QApplication::ColorSpec
    \obsolete

    \value NormalColor the default color allocation policy
    \value CustomColor the same as NormalColor for X11; allocates colors
    to a palette on demand under Windows
    \value ManyColor the right choice for applications that use thousands of
    colors

    See setColorSpec() for full details.
*/
#endif

/*!
    \fn QWidget *QApplication::topLevelAt(const QPoint &point)

    Returns the top-level widget at the given \a point; returns \nullptr if
    there is no such widget.
*/
QWidget *QApplication::topLevelAt(const QPoint &pos)
{
    if (const QWindow *window = QGuiApplication::topLevelAt(pos)) {
        if (const QWidgetWindow *widgetWindow = qobject_cast<const QWidgetWindow *>(window))
            return widgetWindow->widget();
    }
    return nullptr;
}

/*!
    \fn QWidget *QApplication::topLevelAt(int x, int y)

    \overload

    Returns the top-level widget at the point (\a{x}, \a{y}); returns
    0 if there is no such widget.
*/

void qt_init_tooltip_palette();
void qt_cleanup();

QStyle *QApplicationPrivate::app_style = nullptr;        // default application style
#ifndef QT_NO_STYLE_STYLESHEET
QString QApplicationPrivate::styleSheet;           // default application stylesheet
#endif
QPointer<QWidget> QApplicationPrivate::leaveAfterRelease = nullptr;

QFont *QApplicationPrivate::sys_font = nullptr;        // default system font
QFont *QApplicationPrivate::set_font = nullptr;        // default font set by programmer

QWidget *QApplicationPrivate::main_widget = nullptr;        // main application widget
QWidget *QApplicationPrivate::focus_widget = nullptr;        // has keyboard input focus
QWidget *QApplicationPrivate::hidden_focus_widget = nullptr; // will get keyboard input focus after show()
QWidget *QApplicationPrivate::active_window = nullptr;        // toplevel with keyboard focus
#if QT_CONFIG(wheelevent)
QPointer<QWidget> QApplicationPrivate::wheel_widget;
#endif
bool qt_in_tab_key_event = false;
int qt_antialiasing_threshold = -1;
QSize QApplicationPrivate::app_strut = QSize(0,0); // no default application strut
int QApplicationPrivate::enabledAnimations = QPlatformTheme::GeneralUiEffect;
bool QApplicationPrivate::widgetCount = false;
#ifdef QT_KEYPAD_NAVIGATION
Qt::NavigationMode QApplicationPrivate::navigationMode = Qt::NavigationModeKeypadTabOrder;
QWidget *QApplicationPrivate::oldEditFocus = 0;
#endif

inline bool QApplicationPrivate::isAlien(QWidget *widget)
{
    return widget && !widget->isWindow();
}

bool Q_WIDGETS_EXPORT qt_tab_all_widgets()
{
    return QGuiApplication::styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls;
}

// ######## move to QApplicationPrivate
// Default fonts (per widget type)
Q_GLOBAL_STATIC(FontHash, app_fonts)
// Exported accessor for use outside of this file
FontHash *qt_app_fonts_hash() { return app_fonts(); }

QWidgetList *QApplicationPrivate::popupWidgets = nullptr;        // has keyboard input focus

QDesktopWidget *qt_desktopWidget = nullptr;                // root window widgets

/*!
    \internal
*/
void QApplicationPrivate::process_cmdline()
{
    if (styleOverride.isEmpty() && qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE"))
        styleOverride = QString::fromLocal8Bit(qgetenv("QT_STYLE_OVERRIDE"));

    // process platform-indep command line
    if (!qt_is_gui_used || !argc)
        return;

    int i, j;

    j = 1;
    for (i=1; i<argc; i++) { // if you add anything here, modify QCoreApplication::arguments()
        if (!argv[i])
            continue;
        if (*argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        const char *arg = argv[i];
        if (arg[1] == '-') // startsWith("--")
            ++arg;
        if (strcmp(arg, "-qdevel") == 0 || strcmp(arg, "-qdebug") == 0) {
            // obsolete argument
#ifndef QT_NO_STYLE_STYLESHEET
        } else if (strcmp(arg, "-stylesheet") == 0 && i < argc -1) {
            styleSheet = QLatin1String("file:///");
            styleSheet.append(QString::fromLocal8Bit(argv[++i]));
        } else if (strncmp(arg, "-stylesheet=", 12) == 0) {
            styleSheet = QLatin1String("file:///");
            styleSheet.append(QString::fromLocal8Bit(arg + 12));
#endif
        } else if (qstrcmp(arg, "-widgetcount") == 0) {
            widgetCount = true;
        } else {
            argv[j++] = argv[i];
        }
    }

    if(j < argc) {
        argv[j] = nullptr;
        argc = j;
    }
}

/*!
    Initializes the window system and constructs an application object with
    \a argc command line arguments in \a argv.

    \warning The data referred to by \a argc and \a argv must stay valid for
    the entire lifetime of the QApplication object. In addition, \a argc must
    be greater than zero and \a argv must contain at least one valid character
    string.

    The global \c qApp pointer refers to this application object. Only one
    application object should be created.

    This application object must be constructed before any \l{QPaintDevice}
    {paint devices} (including widgets, pixmaps, bitmaps etc.).

    \note \a argc and \a argv might be changed as Qt removes command line
    arguments that it recognizes.

    All Qt programs automatically support the following command line options:
    \list
        \li  -style= \e style, sets the application GUI style. Possible values
            depend on your system configuration. If you compiled Qt with
            additional styles or have additional styles as plugins these will
            be available to the \c -style command line option.  You can also
            set the style for all Qt applications by setting the
            \c QT_STYLE_OVERRIDE environment variable.
        \li  -style \e style, is the same as listed above.
        \li  -stylesheet= \e stylesheet, sets the application \l styleSheet. The
            value must be a path to a file that contains the Style Sheet.
            \note Relative URLs in the Style Sheet file are relative to the
            Style Sheet file's path.
        \li  -stylesheet \e stylesheet, is the same as listed above.
        \li  -widgetcount, prints debug message at the end about number of
            widgets left undestroyed and maximum number of widgets existed at
            the same time
        \li  -reverse, sets the application's layout direction to
            Qt::RightToLeft
        \li  -qmljsdebugger=, activates the QML/JS debugger with a specified port.
            The value must be of format port:1234[,block], where block is optional
            and will make the application wait until a debugger connects to it.
    \endlist

    \sa QCoreApplication::arguments()
*/

#ifdef Q_QDOC
QApplication::QApplication(int &argc, char **argv)
#else
QApplication::QApplication(int &argc, char **argv, int _internal)
#endif
    : QGuiApplication(*new QApplicationPrivate(argc, argv, _internal))
{
    Q_D(QApplication);
    d->init();
}

/*!
    \internal
*/
void QApplicationPrivate::init()
{
#if defined(Q_OS_MACOS)
    QMacAutoReleasePool pool;
#endif

    QGuiApplicationPrivate::init();

    initResources();

    qt_is_gui_used = (application_type != QApplicationPrivate::Tty);
    process_cmdline();

    // Must be called before initialize()
    QColormap::initialize();
    initializeWidgetPalettesFromTheme();
    qt_init_tooltip_palette();
    QApplicationPrivate::initializeWidgetFontHash();

    initialize();
    eventDispatcher->startingUp();

#ifndef QT_NO_ACCESSIBILITY
    // factory for accessible interfaces for widgets shipped with Qt
    QAccessible::installFactory(&qAccessibleFactory);
#endif

}

void qt_init_tooltip_palette()
{
#ifndef QT_NO_TOOLTIP
    if (const QPalette *toolTipPalette = QGuiApplicationPrivate::platformTheme()->palette(QPlatformTheme::ToolTipPalette))
        QToolTip::setPalette(*toolTipPalette);
#endif
}

#if QT_CONFIG(statemachine)
void qRegisterGuiStateMachine();
void qUnregisterGuiStateMachine();
#endif
extern void qRegisterWidgetsVariant();

/*!
  \fn void QApplicationPrivate::initialize()

  Initializes the QApplication object, called from the constructors.
*/
void QApplicationPrivate::initialize()
{
    is_app_running = false; // Starting up.

    QWidgetPrivate::mapper = new QWidgetMapper;
    QWidgetPrivate::allWidgets = new QWidgetSet;

    // needed for a static build.
    qRegisterWidgetsVariant();

    // needed for widgets in QML
    QAbstractDeclarativeData::setWidgetParent = QWidgetPrivate::setWidgetParentHelper;

    if (application_type != QApplicationPrivate::Tty) {
        if (!styleOverride.isEmpty()) {
            if (auto *style = QStyleFactory::create(styleOverride.toLower())) {
                QApplication::setStyle(style);
            } else {
                qWarning("QApplication: invalid style override '%s' passed, ignoring it.\n"
                    "\tAvailable styles: %s", qPrintable(styleOverride),
                    qPrintable(QStyleFactory::keys().join(QLatin1String(", "))));
            }
        }

        // Trigger default style if none was set already
        Q_UNUSED(QApplication::style());
    }
#if QT_CONFIG(statemachine)
    // trigger registering of QStateMachine's GUI types
    qRegisterGuiStateMachine();
#endif

    if (qEnvironmentVariableIntValue("QT_USE_NATIVE_WINDOWS") > 0)
        QCoreApplication::setAttribute(Qt::AA_NativeWindows);

    if (qt_is_gui_used)
        initializeMultitouch();

    if (QGuiApplication::desktopSettingsAware())
        if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
            QApplicationPrivate::enabledAnimations = theme->themeHint(QPlatformTheme::UiEffects).toInt();
        }

    is_app_running = true; // no longer starting up
}

void QApplicationPrivate::initializeWidgetFontHash()
{
    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (!theme)
        return;
    FontHash *fontHash = app_fonts();
    fontHash->clear();

    if (const QFont *font = theme->font(QPlatformTheme::MenuFont))
        fontHash->insert(QByteArrayLiteral("QMenu"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MenuBarFont))
        fontHash->insert(QByteArrayLiteral("QMenuBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MenuItemFont))
        fontHash->insert(QByteArrayLiteral("QMenuItem"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MessageBoxFont))
        fontHash->insert(QByteArrayLiteral("QMessageBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::LabelFont))
        fontHash->insert(QByteArrayLiteral("QLabel"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::TipLabelFont))
        fontHash->insert(QByteArrayLiteral("QTipLabel"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::TitleBarFont))
        fontHash->insert(QByteArrayLiteral("QTitleBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::StatusBarFont))
        fontHash->insert(QByteArrayLiteral("QStatusBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MdiSubWindowTitleFont))
        fontHash->insert(QByteArrayLiteral("QMdiSubWindowTitleBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::DockWidgetTitleFont))
        fontHash->insert(QByteArrayLiteral("QDockWidgetTitle"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::PushButtonFont))
        fontHash->insert(QByteArrayLiteral("QPushButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::CheckBoxFont))
        fontHash->insert(QByteArrayLiteral("QCheckBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::RadioButtonFont))
        fontHash->insert(QByteArrayLiteral("QRadioButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ToolButtonFont))
        fontHash->insert(QByteArrayLiteral("QToolButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ItemViewFont))
        fontHash->insert(QByteArrayLiteral("QAbstractItemView"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ListViewFont))
        fontHash->insert(QByteArrayLiteral("QListView"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::HeaderViewFont))
        fontHash->insert(QByteArrayLiteral("QHeaderView"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ListBoxFont))
        fontHash->insert(QByteArrayLiteral("QListBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ComboMenuItemFont))
        fontHash->insert(QByteArrayLiteral("QComboMenuItem"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ComboLineEditFont))
        fontHash->insert(QByteArrayLiteral("QComboLineEdit"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::SmallFont))
        fontHash->insert(QByteArrayLiteral("QSmallFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MiniFont))
        fontHash->insert(QByteArrayLiteral("QMiniFont"), *font);
}

/*****************************************************************************
  Functions returning the active popup and modal widgets.
 *****************************************************************************/

/*!
    Returns the active popup widget.

    A popup widget is a special top-level widget that sets the \c
    Qt::WType_Popup widget flag, e.g. the QMenu widget. When the application
    opens a popup widget, all events are sent to the popup. Normal widgets and
    modal widgets cannot be accessed before the popup widget is closed.

    Only other popup widgets may be opened when a popup widget is shown. The
    popup widgets are organized in a stack. This function returns the active
    popup widget at the top of the stack.

    \sa activeModalWidget(), topLevelWidgets()
*/

QWidget *QApplication::activePopupWidget()
{
    return QApplicationPrivate::popupWidgets && !QApplicationPrivate::popupWidgets->isEmpty() ?
        QApplicationPrivate::popupWidgets->constLast() : nullptr;
}


/*!
    Returns the active modal widget.

    A modal widget is a special top-level widget which is a subclass of QDialog
    that specifies the modal parameter of the constructor as true. A modal
    widget must be closed before the user can continue with other parts of the
    program.

    Modal widgets are organized in a stack. This function returns the active
    modal widget at the top of the stack.

    \sa activePopupWidget(), topLevelWidgets()
*/

QWidget *QApplication::activeModalWidget()
{
    QWidgetWindow *widgetWindow = qobject_cast<QWidgetWindow *>(modalWindow());
    return widgetWindow ? widgetWindow->widget() : nullptr;
}

/*!
    Cleans up any window system resources that were allocated by this
    application. Sets the global variable \c qApp to \nullptr.
*/

QApplication::~QApplication()
{
    Q_D(QApplication);

    //### this should probable be done even later
    qt_call_post_routines();

    // kill timers before closing down the dispatcher
    d->toolTipWakeUp.stop();
    d->toolTipFallAsleep.stop();

    QApplicationPrivate::is_app_closing = true;
    QApplicationPrivate::is_app_running = false;

    delete QWidgetPrivate::mapper;
    QWidgetPrivate::mapper = nullptr;

    // delete all widgets
    if (QWidgetPrivate::allWidgets) {
        QWidgetSet *mySet = QWidgetPrivate::allWidgets;
        QWidgetPrivate::allWidgets = nullptr;
        for (QWidgetSet::ConstIterator it = mySet->constBegin(), cend = mySet->constEnd(); it != cend; ++it) {
            QWidget *w = *it;
            if (!w->parent())                        // window
                w->destroy(true, true);
        }
        delete mySet;
    }

    delete qt_desktopWidget;
    qt_desktopWidget = nullptr;

    QApplicationPrivate::widgetPalettes.clear();

    delete QApplicationPrivate::sys_font;
    QApplicationPrivate::sys_font = nullptr;
    delete QApplicationPrivate::set_font;
    QApplicationPrivate::set_font = nullptr;
    app_fonts()->clear();

    delete QApplicationPrivate::app_style;
    QApplicationPrivate::app_style = nullptr;

#if QT_CONFIG(draganddrop)
    if (qt_is_gui_used)
        delete QDragManager::self();
#endif

    d->cleanupMultitouch();

    qt_cleanup();

    if (QApplicationPrivate::widgetCount)
        qDebug("Widgets left: %i    Max widgets: %i \n", QWidgetPrivate::instanceCounter, QWidgetPrivate::maxInstances);

    QApplicationPrivate::obey_desktop_settings = true;

    QApplicationPrivate::app_strut = QSize(0, 0);
    QApplicationPrivate::enabledAnimations = QPlatformTheme::GeneralUiEffect;
    QApplicationPrivate::widgetCount = false;

#if QT_CONFIG(statemachine)
    // trigger unregistering of QStateMachine's GUI types
    qUnregisterGuiStateMachine();
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
// #fixme: Remove.
static HDC         displayDC        = 0;                // display device context

Q_WIDGETS_EXPORT HDC qt_win_display_dc()                        // get display DC
{
    Q_ASSERT(qApp && qApp->thread() == QThread::currentThread());
    if (!displayDC)
        displayDC = GetDC(0);
    return displayDC;
}
#endif
#endif

void qt_cleanup()
{
    QPixmapCache::clear();
    QColormap::cleanup();

    QApplicationPrivate::active_window = nullptr; //### this should not be necessary
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    if (displayDC) {
        ReleaseDC(0, displayDC);
        displayDC = 0;
    }
#endif
#endif
}

/*!
    \fn QWidget *QApplication::widgetAt(const QPoint &point)

    Returns the widget at global screen position \a point, or \nullptr
    if there is no Qt widget there.

    This function can be slow.

    \sa QCursor::pos(), QWidget::grabMouse(), QWidget::grabKeyboard()
*/
QWidget *QApplication::widgetAt(const QPoint &p)
{
    QWidget *window = QApplication::topLevelAt(p);
    if (!window)
        return nullptr;

    QWidget *child = nullptr;

    if (!window->testAttribute(Qt::WA_TransparentForMouseEvents))
        child = window->childAt(window->mapFromGlobal(p));

    if (child)
        return child;

    if (window->testAttribute(Qt::WA_TransparentForMouseEvents)) {
        //shoot a hole in the widget and try once again,
        //suboptimal on Qt for Embedded Linux where we do
        //know the stacking order of the toplevels.
        int x = p.x();
        int y = p.y();
        QRegion oldmask = window->mask();
        QPoint wpoint = window->mapFromGlobal(QPoint(x, y));
        QRegion newmask = (oldmask.isEmpty() ? QRegion(window->rect()) : oldmask)
                          - QRegion(wpoint.x(), wpoint.y(), 1, 1);
        window->setMask(newmask);
        QWidget *recurse = nullptr;
        if (QApplication::topLevelAt(p) != window) // verify recursion will terminate
            recurse = widgetAt(x, y);
        if (oldmask.isEmpty())
            window->clearMask();
        else
            window->setMask(oldmask);
        return recurse;
    }
    return window;
}

/*!
    \fn QWidget *QApplication::widgetAt(int x, int y)

    \overload

    Returns the widget at global screen position (\a x, \a y), or
    \nullptr if there is no Qt widget there.
*/

/*!
    \internal
*/
bool QApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    if ((event->type() == QEvent::UpdateRequest
          || event->type() == QEvent::LayoutRequest
          || event->type() == QEvent::Resize
          || event->type() == QEvent::Move
          || event->type() == QEvent::LanguageChange)) {
        for (QPostEventList::const_iterator it = postedEvents->constBegin(); it != postedEvents->constEnd(); ++it) {
            const QPostEvent &cur = *it;
            if (cur.receiver != receiver || cur.event == nullptr || cur.event->type() != event->type())
                continue;
            if (cur.event->type() == QEvent::LayoutRequest
                 || cur.event->type() == QEvent::UpdateRequest) {
                ;
            } else if (cur.event->type() == QEvent::Resize) {
                ((QResizeEvent *)(cur.event))->s = ((QResizeEvent *)event)->s;
            } else if (cur.event->type() == QEvent::Move) {
                ((QMoveEvent *)(cur.event))->p = ((QMoveEvent *)event)->p;
            } else if (cur.event->type() == QEvent::LanguageChange) {
                ;
            } else {
                continue;
            }
            delete event;
            return true;
        }
        return false;
    }
    return QGuiApplication::compressEvent(event, receiver, postedEvents);
}

/*!
    \property QApplication::styleSheet
    \brief the application style sheet
    \since 4.2

    By default, this property returns an empty string unless the user specifies
    the \c{-stylesheet} option on the command line when running the application.

    \sa QWidget::setStyle(), {Qt Style Sheets}
*/

/*!
    \property QApplication::autoSipEnabled
    \since 4.5
    \brief toggles automatic SIP (software input panel) visibility

    Set this property to \c true to automatically display the SIP when entering
    widgets that accept keyboard input. This property only affects widgets with
    the WA_InputMethodEnabled attribute set, and is typically used to launch
    a virtual keyboard on devices which have very few or no keys.

    \b{ The property only has an effect on platforms that use software input
    panels.}

    The default is platform dependent.
*/
void QApplication::setAutoSipEnabled(const bool enabled)
{
    QApplicationPrivate::autoSipEnabled = enabled;
}

bool QApplication::autoSipEnabled() const
{
    return QApplicationPrivate::autoSipEnabled;
}

#ifndef QT_NO_STYLE_STYLESHEET

QString QApplication::styleSheet() const
{
    return QApplicationPrivate::styleSheet;
}

void QApplication::setStyleSheet(const QString& styleSheet)
{
    QApplicationPrivate::styleSheet = styleSheet;
    QStyleSheetStyle *styleSheetStyle = qt_styleSheet(QApplicationPrivate::app_style);
    if (styleSheet.isEmpty()) { // application style sheet removed
        if (!styleSheetStyle)
            return; // there was no stylesheet before
        setStyle(styleSheetStyle->base);
    } else if (styleSheetStyle) { // style sheet update, just repolish
        styleSheetStyle->repolish(qApp);
    } else { // stylesheet set the first time
        QStyleSheetStyle *newStyleSheetStyle = new QStyleSheetStyle(QApplicationPrivate::app_style);
        QApplicationPrivate::app_style->setParent(newStyleSheetStyle);
        setStyle(newStyleSheetStyle);
    }
}

#endif // QT_NO_STYLE_STYLESHEET

/*!
    Returns the application's style object.

    \sa setStyle(), QStyle
*/
QStyle *QApplication::style()
{
    if (!QApplicationPrivate::app_style) {
        // Create default style
        if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
            Q_ASSERT(!"No style available without QApplication!");
            return nullptr;
        }

        auto &defaultStyle = QApplicationPrivate::app_style;

        defaultStyle = QStyleFactory::create(QApplicationPrivate::desktopStyleKey());
        if (!defaultStyle) {
            const QStringList styles = QStyleFactory::keys();
            for (const auto &style : styles) {
                if ((defaultStyle = QStyleFactory::create(style)))
                    break;
            }
        }
        if (!defaultStyle) {
            Q_ASSERT(!"No styles available!");
            return nullptr;
        }

        // Take ownership of the style
        defaultStyle->setParent(qApp);

        QGuiApplicationPrivate::updatePalette();

#ifndef QT_NO_STYLE_STYLESHEET
        if (!QApplicationPrivate::styleSheet.isEmpty()) {
            qApp->setStyleSheet(QApplicationPrivate::styleSheet);
        } else
#endif
        {
            defaultStyle->polish(qApp);
        }
    }

    return QApplicationPrivate::app_style;
}

/*!
    Sets the application's GUI style to \a style. Ownership of the style object
    is transferred to QApplication, so QApplication will delete the style
    object on application exit or when a new style is set and the old style is
    still the parent of the application object.

    Example usage:
    \snippet code/src_gui_kernel_qapplication.cpp 1

    When switching application styles, the color palette is set back to the
    initial colors or the system defaults. This is necessary since certain
    styles have to adapt the color palette to be fully style-guide compliant.

    Setting the style before a palette has been set, i.e., before creating
    QApplication, will cause the application to use QStyle::standardPalette()
    for the palette.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa style(), QStyle, setPalette(), desktopSettingsAware()
*/
void QApplication::setStyle(QStyle *style)
{
    if (!style || style == QApplicationPrivate::app_style)
        return;

    QWidgetList all = allWidgets();

    // clean up the old style
    if (QApplicationPrivate::app_style) {
        if (QApplicationPrivate::is_app_running && !QApplicationPrivate::is_app_closing) {
            for (QWidgetList::ConstIterator it = all.constBegin(), cend = all.constEnd(); it != cend; ++it) {
                QWidget *w = *it;
                if (!(w->windowType() == Qt::Desktop) &&        // except desktop
                     w->testAttribute(Qt::WA_WState_Polished)) { // has been polished
                    QApplicationPrivate::app_style->unpolish(w);
                }
            }
        }
        QApplicationPrivate::app_style->unpolish(qApp);
    }

    QStyle *old = QApplicationPrivate::app_style; // save

#ifndef QT_NO_STYLE_STYLESHEET
    if (!QApplicationPrivate::styleSheet.isEmpty() && !qt_styleSheet(style)) {
        // we have a stylesheet already and a new style is being set
        QStyleSheetStyle *newStyleSheetStyle = new QStyleSheetStyle(style);
        style->setParent(newStyleSheetStyle);
        QApplicationPrivate::app_style = newStyleSheetStyle;
    } else
#endif // QT_NO_STYLE_STYLESHEET
        QApplicationPrivate::app_style = style;
    QApplicationPrivate::app_style->setParent(qApp); // take ownership

    // Take care of possible palette requirements of certain
    // styles. Do it before polishing the application since the
    // style might call QApplication::setPalette() itself.
    QGuiApplicationPrivate::updatePalette();

    // The default widget font hash is based on the platform theme,
    // not the style, but the widget fonts could in theory have been
    // affected by polish of the previous style, without a proper
    // cleanup in unpolish, so reset it now before polishing the
    // new style.
    QApplicationPrivate::initializeWidgetFontHash();

    // initialize the application with the new style
    QApplicationPrivate::app_style->polish(qApp);

    // re-polish existing widgets if necessary
    if (QApplicationPrivate::is_app_running && !QApplicationPrivate::is_app_closing) {
        for (QWidgetList::ConstIterator it = all.constBegin(), cend = all.constEnd(); it != cend; ++it) {
            QWidget *w = *it;
            if (w->windowType() != Qt::Desktop && w->testAttribute(Qt::WA_WState_Polished)) {
                if (w->style() == QApplicationPrivate::app_style)
                    QApplicationPrivate::app_style->polish(w);                // repolish
#ifndef QT_NO_STYLE_STYLESHEET
                else
                    w->setStyleSheet(w->styleSheet()); // touch
#endif
            }
        }

        for (QWidgetList::ConstIterator it = all.constBegin(), cend = all.constEnd(); it != cend; ++it) {
            QWidget *w = *it;
            if (w->windowType() != Qt::Desktop && !w->testAttribute(Qt::WA_SetStyle)) {
                    QEvent e(QEvent::StyleChange);
                    QCoreApplication::sendEvent(w, &e);
                    w->update();
            }
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (QStyleSheetStyle *oldStyleSheetStyle = qt_styleSheet(old)) {
        oldStyleSheetStyle->deref();
    } else
#endif
    if (old && old->parent() == qApp) {
        delete old;
    }

    if (QApplicationPrivate::focus_widget) {
        QFocusEvent in(QEvent::FocusIn, Qt::OtherFocusReason);
        QCoreApplication::sendEvent(QApplicationPrivate::focus_widget->style(), &in);
        QApplicationPrivate::focus_widget->update();
    }
}

/*!
    \overload

    Requests a QStyle object for \a style from the QStyleFactory.

    The string must be one of the QStyleFactory::keys(), typically one of
    "windows", "windowsvista", "fusion", or "macintosh". Style
    names are case insensitive.

    Returns \nullptr if an unknown \a style is passed, otherwise the QStyle object
    returned is set as the application's GUI style.

    \warning To ensure that the application's style is set correctly, it is
    best to call this function before the QApplication constructor, if
    possible.
*/
QStyle* QApplication::setStyle(const QString& style)
{
    QStyle *s = QStyleFactory::create(style);
    if (!s)
        return nullptr;

    setStyle(s);
    return s;
}

#if QT_DEPRECATED_SINCE(5, 8)
/*!
    Returns the color specification.
    \obsolete

    \sa QApplication::setColorSpec()
*/

int QApplication::colorSpec()
{
    return QApplication::NormalColor;
}

/*!
    Sets the color specification for the application to \a spec.
    \obsolete

    This call has no effect.

    The color specification controls how the application allocates colors when
    run on a display with a limited amount of colors, e.g. 8 bit / 256 color
    displays.

    The color specification must be set before you create the QApplication
    object.

    The options are:
    \list
        \li  QApplication::NormalColor. This is the default color allocation
            strategy. Use this option if your application uses buttons, menus,
            texts and pixmaps with few colors. With this option, the
            application uses system global colors. This works fine for most
            applications under X11, but on the Windows platform, it may cause
            dithering of non-standard colors.
        \li  QApplication::CustomColor. Use this option if your application
            needs a small number of custom colors. On X11, this option is the
            same as NormalColor. On Windows, Qt creates a Windows palette, and
            allocates colors to it on demand.
        \li  QApplication::ManyColor. Use this option if your application is
            very color hungry, e.g., it requires thousands of colors. \br
            Under X11 the effect is:
            \list
                \li  For 256-color displays which have at best a 256 color true
                    color visual, the default visual is used, and colors are
                    allocated from a color cube. The color cube is the 6x6x6
                    (216 color) "Web palette" (the red, green, and blue
                    components always have one of the following values: 0x00,
                    0x33, 0x66, 0x99, 0xCC, or 0xFF), but the number of colors
                    can be changed by the \e -ncols option. The user can force
                    the application to use the true color visual with the
                    \l{QApplication::QApplication()}{-visual} option.
                \li  For 256-color displays which have a true color visual with
                    more than 256 colors, use that visual. Silicon Graphics X
                    servers this feature, for example. They provide an 8 bit
                    visual by default but can deliver true color when asked.
            \endlist
            On Windows, Qt creates a Windows palette, and fills it with a color
            cube.
    \endlist

    Be aware that the CustomColor and ManyColor choices may lead to colormap
    flashing: The foreground application gets (most) of the available colors,
    while the background windows will look less attractive.

    Example:

    \snippet code/src_gui_kernel_qapplication.cpp 2

    \sa colorSpec()
*/

void QApplication::setColorSpec(int spec)
{
    Q_UNUSED(spec)
}
#endif

/*!
    \property QApplication::globalStrut
    \brief the minimum size that any GUI element that the user can interact
           with should have
    \deprecated

    For example, no button should be resized to be smaller than the global
    strut size. The strut size should be considered when reimplementing GUI
    controls that may be used on touch-screens or similar I/O devices.

    Example:

    \snippet code/src_gui_kernel_qapplication.cpp 3

    By default, this property contains a QSize object with zero width and height.
*/
QSize QApplication::globalStrut()
{
    return QApplicationPrivate::app_strut;
}

void QApplication::setGlobalStrut(const QSize& strut)
{
    QApplicationPrivate::app_strut = strut;
}

// Widget specific palettes
QApplicationPrivate::PaletteHash QApplicationPrivate::widgetPalettes;

QPalette QApplicationPrivate::basePalette() const
{
    // Start out with a palette based on the style, in case there's no theme
    // available, or so that we can fill in missing roles in the theme.
    QPalette palette = app_style ? app_style->standardPalette() : Qt::gray;

    // Prefer theme palette if available, but fill in missing roles from style
    // for compatibility. Note that the style's standard palette is not prioritized
    // over the theme palette, as the documented way of applying the style's palette
    // is to set it explicitly using QApplication::setPalette().
    if (const QPalette *themePalette = platformTheme() ? platformTheme()->palette() : nullptr)
        palette = themePalette->resolve(palette);

    // Finish off by letting the application style polish the palette. This will
    // not result in the polished palette becoming a user-set palette, as the
    // resulting base palette is only used as a fallback, with the resolve mask
    // set to 0.
    if (app_style)
        app_style->polish(palette);

    return palette;
}

/*!
    \fn QPalette QApplication::palette(const QWidget* widget)

    If a \a widget is passed, the default palette for the widget's class is
    returned. This may or may not be the application palette. In most cases
    there is no special palette for certain types of widgets, but one notable
    exception is the popup menu under Windows, if the user has defined a
    special background color for menus in the display settings.

    \sa setPalette(), QWidget::palette()
*/
QPalette QApplication::palette(const QWidget* w)
{
    auto &widgetPalettes = QApplicationPrivate::widgetPalettes;
    if (w && !widgetPalettes.isEmpty()) {
        auto it = widgetPalettes.constFind(w->metaObject()->className());
        const auto cend = widgetPalettes.constEnd();
        if (it != cend)
            return *it;
        for (it = widgetPalettes.constBegin(); it != cend; ++it) {
            if (w->inherits(it.key()))
                return it.value();
        }
    }
    return palette();
}

/*!
    \overload

    Returns the palette for widgets of the given \a className.

    \sa setPalette(), QWidget::palette()
*/
QPalette QApplication::palette(const char *className)
{
    auto &widgetPalettes = QApplicationPrivate::widgetPalettes;
    if (className && !widgetPalettes.isEmpty()) {
        auto it = widgetPalettes.constFind(className);
        if (it != widgetPalettes.constEnd())
            return *it;
    }

    return QGuiApplication::palette();
}

/*!
    Changes the application palette to \a palette.

    If \a className is passed, the change applies only to widgets that inherit
    \a className (as reported by QObject::inherits()). If \a className is left
    0, the change affects all widgets, thus overriding any previously set class
    specific palettes.

    The palette may be changed according to the current GUI style in
    QStyle::polish().

    \warning Do not use this function in conjunction with \l{Qt Style Sheets}.
    When using style sheets, the palette of a widget can be customized using
    the "color", "background-color", "selection-color",
    "selection-background-color" and "alternate-background-color".

    \note Some styles do not use the palette for all drawing, for instance, if
    they make use of native theme engines. This is the case for the
    Windows Vista and \macos styles.

    \sa QWidget::setPalette(), palette(), QStyle::polish()
*/
void QApplication::setPalette(const QPalette &palette, const char* className)
{
    if (className) {
        QPalette polishedPalette = palette;
        if (QApplicationPrivate::app_style) {
            auto originalResolveMask = palette.resolve();
            QApplicationPrivate::app_style->polish(polishedPalette);
            polishedPalette.resolve(originalResolveMask);
        }

        QApplicationPrivate::widgetPalettes.insert(className, polishedPalette);
        if (qApp)
            qApp->d_func()->handlePaletteChanged(className);
    } else {
        QGuiApplication::setPalette(palette);
    }
}

void QApplicationPrivate::handlePaletteChanged(const char *className)
{
    if (!is_app_running || is_app_closing)
        return;

    // Setting the global application palette is documented to
    // reset any previously set class specific widget palettes.
    bool sendPaletteChangeToAllWidgets = false;
    if (!className && !widgetPalettes.isEmpty()) {
        sendPaletteChangeToAllWidgets = true;
        widgetPalettes.clear();
    }

    QGuiApplicationPrivate::handlePaletteChanged(className);

    QEvent event(QEvent::ApplicationPaletteChange);
    const QWidgetList widgets = QApplication::allWidgets();
    for (auto widget : widgets) {
        if (sendPaletteChangeToAllWidgets || (!className && widget->isWindow()) || (className && widget->inherits(className)))
            QCoreApplication::sendEvent(widget, &event);
    }

#if QT_CONFIG(graphicsview)
    for (auto scene : qAsConst(scene_list))
        QCoreApplication::sendEvent(scene, &event);
#endif

    // Palette has been reset back to the default application palette,
    // so we need to reinitialize the widget palettes from the theme.
    if (!className && !testAttribute(Qt::AA_SetPalette))
        initializeWidgetPalettesFromTheme();
}

void QApplicationPrivate::initializeWidgetPalettesFromTheme()
{
    QPlatformTheme *platformTheme = QGuiApplicationPrivate::platformTheme();
    if (!platformTheme)
        return;

    widgetPalettes.clear();

    struct ThemedWidget { const char *className; QPlatformTheme::Palette palette; };

    static const ThemedWidget themedWidgets[] = {
        { "QToolButton", QPlatformTheme::ToolButtonPalette },
        { "QAbstractButton", QPlatformTheme::ButtonPalette },
        { "QCheckBox", QPlatformTheme::CheckBoxPalette },
        { "QRadioButton", QPlatformTheme::RadioButtonPalette },
        { "QHeaderView", QPlatformTheme::HeaderPalette },
        { "QAbstractItemView", QPlatformTheme::ItemViewPalette },
        { "QMessageBoxLabel", QPlatformTheme::MessageBoxLabelPalette },
        { "QTabBar", QPlatformTheme::TabBarPalette },
        { "QLabel", QPlatformTheme::LabelPalette },
        { "QGroupBox", QPlatformTheme::GroupBoxPalette },
        { "QMenu", QPlatformTheme::MenuPalette },
        { "QMenuBar", QPlatformTheme::MenuBarPalette },
        { "QTextEdit", QPlatformTheme::TextEditPalette },
        { "QTextControl", QPlatformTheme::TextEditPalette },
        { "QLineEdit", QPlatformTheme::TextLineEditPalette },
    };

    for (const auto themedWidget : themedWidgets) {
        if (auto *palette = platformTheme->palette(themedWidget.palette))
            QApplication::setPalette(*palette, themedWidget.className);
    }
}

/*!
    Returns the default application font.

    \sa fontMetrics(), QWidget::font()
*/
QFont QApplication::font()
{
    return QGuiApplication::font();
}

/*!
    \overload

    Returns the default font for the \a widget.

    \sa fontMetrics(), QWidget::setFont()
*/

QFont QApplication::font(const QWidget *widget)
{
    typedef FontHash::const_iterator FontHashConstIt;

    FontHash *hash = app_fonts();

    if (widget && hash  && hash->size()) {
#ifdef Q_OS_MAC
        // short circuit for small and mini controls
        if (widget->testAttribute(Qt::WA_MacSmallSize)) {
            return hash->value(QByteArrayLiteral("QSmallFont"));
        } else if (widget->testAttribute(Qt::WA_MacMiniSize)) {
            return hash->value(QByteArrayLiteral("QMiniFont"));
        }
#endif
        FontHashConstIt it = hash->constFind(widget->metaObject()->className());
        const FontHashConstIt cend = hash->constEnd();
        if (it != cend)
            return it.value();
        for (it = hash->constBegin(); it != cend; ++it) {
            if (widget->inherits(it.key()))
                return it.value();
        }
    }
    return font();
}

/*!
    \overload

    Returns the font for widgets of the given \a className.

    \sa setFont(), QWidget::font()
*/
QFont QApplication::font(const char *className)
{
    FontHash *hash = app_fonts();
    if (className && hash && hash->size()) {
        QHash<QByteArray, QFont>::ConstIterator it = hash->constFind(className);
        if (it != hash->constEnd())
            return *it;
    }
    return font();
}


/*!
    Changes the default application font to \a font. If \a className is passed,
    the change applies only to classes that inherit \a className (as reported
    by QObject::inherits()).

    On application start-up, the default font depends on the window system. It
    can vary depending on both the window system version and the locale. This
    function lets you override the default font; but overriding may be a bad
    idea because, for example, some locales need extra large fonts to support
    their special characters.

    \warning Do not use this function in conjunction with \l{Qt Style Sheets}.
    The font of an application can be customized using the "font" style sheet
    property. To set a bold font for all QPushButtons, set the application
    styleSheet() as "QPushButton { font: bold }"

    \sa font(), fontMetrics(), QWidget::setFont()
*/

void QApplication::setFont(const QFont &font, const char *className)
{
    bool all = false;
    FontHash *hash = app_fonts();
    if (!className) {
        QGuiApplication::setFont(font);
        if (hash && hash->size()) {
            all = true;
            hash->clear();
        }
    } else if (hash) {
        hash->insert(className, font);
    }
    if (QApplicationPrivate::is_app_running && !QApplicationPrivate::is_app_closing) {
        // Send ApplicationFontChange to qApp itself, and to the widgets.
        QEvent e(QEvent::ApplicationFontChange);
        QCoreApplication::sendEvent(QApplication::instance(), &e);

        QWidgetList wids = QApplication::allWidgets();
        for (QWidgetList::ConstIterator it = wids.constBegin(), cend = wids.constEnd(); it != cend; ++it) {
            QWidget *w = *it;
            if (all || (!className && w->isWindow()) || w->inherits(className)) // matching class
                sendEvent(w, &e);
        }

#if QT_CONFIG(graphicsview)
        // Send to all scenes as well.
        QList<QGraphicsScene *> &scenes = qApp->d_func()->scene_list;
        for (QList<QGraphicsScene *>::ConstIterator it = scenes.constBegin();
             it != scenes.constEnd(); ++it) {
            QCoreApplication::sendEvent(*it, &e);
        }
#endif // QT_CONFIG(graphicsview)
    }
    if (!className && (!QApplicationPrivate::sys_font || !font.isCopyOf(*QApplicationPrivate::sys_font))) {
        if (!QApplicationPrivate::set_font)
            QApplicationPrivate::set_font = new QFont(font);
        else
            *QApplicationPrivate::set_font = font;
    }
}

/*! \internal
*/
void QApplicationPrivate::setSystemFont(const QFont &font)
{
     if (!sys_font)
        sys_font = new QFont(font);
    else
        *sys_font = font;

    if (!QApplicationPrivate::set_font)
        QApplication::setFont(*sys_font);
}

/*! \internal
*/
QString QApplicationPrivate::desktopStyleKey()
{
#if defined(QT_BUILD_INTERNAL)
    // Allow auto-tests to override the desktop style
    if (qEnvironmentVariableIsSet("QT_DESKTOP_STYLE_KEY"))
        return QString::fromLocal8Bit(qgetenv("QT_DESKTOP_STYLE_KEY"));
#endif

    // The platform theme might return a style that is not available, find
    // first valid one.
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QStringList availableKeys = QStyleFactory::keys();
        const auto styles = theme->themeHint(QPlatformTheme::StyleNames).toStringList();
        for (const QString &style : styles) {
            if (availableKeys.contains(style, Qt::CaseInsensitive))
                return style;
        }
    }
    return QString();
}

#if QT_VERSION < 0x060000 // remove these forwarders in Qt 6
/*!
    \property QApplication::windowIcon
    \brief the default window icon

    \sa QWidget::setWindowIcon(), {Setting the Application Icon}
*/
QIcon QApplication::windowIcon()
{
    return QGuiApplication::windowIcon();
}

void QApplication::setWindowIcon(const QIcon &icon)
{
    QGuiApplication::setWindowIcon(icon);
}
#endif

void QApplicationPrivate::notifyWindowIconChanged()
{
    QEvent ev(QEvent::ApplicationWindowIconChange);
    const QWidgetList list = QApplication::topLevelWidgets();
    QWindowList windowList = QGuiApplication::topLevelWindows();

    // send to all top-level QWidgets
    for (auto *w : list) {
        windowList.removeOne(w->windowHandle());
        QCoreApplication::sendEvent(w, &ev);
    }

    // in case there are any plain QWindows in this QApplication-using
    // application, also send the notification to them
    for (int i = 0; i < windowList.size(); ++i)
        QCoreApplication::sendEvent(windowList.at(i), &ev);
}

/*!
    Returns a list of the top-level widgets (windows) in the application.

    \note Some of the top-level widgets may be hidden, for example a tooltip if
    no tooltip is currently shown.

    Example:

    \snippet code/src_gui_kernel_qapplication.cpp 4

    \sa allWidgets(), QWidget::isWindow(), QWidget::isHidden()
*/
QWidgetList QApplication::topLevelWidgets()
{
    QWidgetList list;
    if (QWidgetPrivate::allWidgets != nullptr) {
        const auto isTopLevelWidget = [] (const QWidget *w) {
            return w->isWindow() && w->windowType() != Qt::Desktop;
        };
        std::copy_if(QWidgetPrivate::allWidgets->cbegin(), QWidgetPrivate::allWidgets->cend(),
                     std::back_inserter(list), isTopLevelWidget);
    }
    return list;
}

/*!
    Returns a list of all the widgets in the application.

    The list is empty (QList::isEmpty()) if there are no widgets.

    \note Some of the widgets may be hidden.

    Example:
    \snippet code/src_gui_kernel_qapplication.cpp 5

    \sa topLevelWidgets(), QWidget::isVisible()
*/

QWidgetList QApplication::allWidgets()
{
    if (QWidgetPrivate::allWidgets)
        return QWidgetPrivate::allWidgets->values();
    return QWidgetList();
}

/*!
    Returns the application widget that has the keyboard input focus,
    or \nullptr if no widget in this application has the focus.

    \sa QWidget::setFocus(), QWidget::hasFocus(), activeWindow(), focusChanged()
*/

QWidget *QApplication::focusWidget()
{
    return QApplicationPrivate::focus_widget;
}

void QApplicationPrivate::setFocusWidget(QWidget *focus, Qt::FocusReason reason)
{
#if QT_CONFIG(graphicsview)
    if (focus && focus->window()->graphicsProxyWidget())
        return;
#endif

    hidden_focus_widget = nullptr;

    if (focus != focus_widget) {
        if (focus && focus->isHidden()) {
            hidden_focus_widget = focus;
            return;
        }

        if (focus && (reason == Qt::BacktabFocusReason || reason == Qt::TabFocusReason)
            && qt_in_tab_key_event)
            focus->window()->setAttribute(Qt::WA_KeyboardFocusChange);
        else if (focus && reason == Qt::ShortcutFocusReason) {
            focus->window()->setAttribute(Qt::WA_KeyboardFocusChange);
        }
        QWidget *prev = focus_widget;
        focus_widget = focus;

        if(focus_widget)
            focus_widget->d_func()->setFocus_sys();

        if (reason != Qt::NoFocusReason) {

            //send events
            if (prev) {
#ifdef QT_KEYPAD_NAVIGATION
                if (QApplication::keypadNavigationEnabled()) {
                    if (prev->hasEditFocus() && reason != Qt::PopupFocusReason)
                        prev->setEditFocus(false);
                }
#endif
                QFocusEvent out(QEvent::FocusOut, reason);
                QPointer<QWidget> that = prev;
                QCoreApplication::sendEvent(prev, &out);
                if (that)
                    QCoreApplication::sendEvent(that->style(), &out);
            }
            if(focus && QApplicationPrivate::focus_widget == focus) {
                QFocusEvent in(QEvent::FocusIn, reason);
                QPointer<QWidget> that = focus;
                QCoreApplication::sendEvent(focus, &in);
                if (that)
                    QCoreApplication::sendEvent(that->style(), &in);
            }
            emit qApp->focusChanged(prev, focus_widget);
        }
    }
}


/*!
    Returns the application top-level window that has the keyboard input focus,
    or \nullptr if no application window has the focus. There might be an
    activeWindow() even if there is no focusWidget(), for example if no widget
    in that window accepts key events.

    \sa QWidget::setFocus(), QWidget::hasFocus(), focusWidget()
*/

QWidget *QApplication::activeWindow()
{
    return QApplicationPrivate::active_window;
}

/*!
    Returns display (screen) font metrics for the application font.

    \sa font(), setFont(), QWidget::fontMetrics(), QPainter::fontMetrics()
*/

QFontMetrics QApplication::fontMetrics()
{
    return desktop()->fontMetrics();
}

bool QApplicationPrivate::tryCloseAllWidgetWindows(QWindowList *processedWindows)
{
    Q_ASSERT(processedWindows);
    while (QWidget *w = QApplication::activeModalWidget()) {
        if (!w->isVisible() || w->data->is_closing)
            break;
        QWindow *window = w->windowHandle();
        if (!window->close()) // Qt::WA_DeleteOnClose may cause deletion.
            return false;
        if (window)
            processedWindows->append(window);
    }

retry:
    const QWidgetList list = QApplication::topLevelWidgets();
    for (auto *w : list) {
        if (w->isVisible() && w->windowType() != Qt::Desktop &&
                !w->testAttribute(Qt::WA_DontShowOnScreen) && !w->data->is_closing) {
            QWindow *window = w->windowHandle();
            if (!window->close())  // Qt::WA_DeleteOnClose may cause deletion.
                return false;
            if (window)
                processedWindows->append(window);
            goto retry;
        }
    }
    return true;
}

bool QApplicationPrivate::tryCloseAllWindows()
{
    QWindowList processedWindows;
    return QApplicationPrivate::tryCloseAllWidgetWindows(&processedWindows)
        && QGuiApplicationPrivate::tryCloseRemainingWindows(processedWindows);
}

/*!
    Closes all top-level windows.

    This function is particularly useful for applications with many top-level
    windows. It could, for example, be connected to a \uicontrol{Exit} entry in the
    \uicontrol{File} menu:

    \snippet mainwindows/mdi/mainwindow.cpp 0

    The windows are closed in random order, until one window does not accept
    the close event. The application quits when the last window was
    successfully closed; this can be turned off by setting
    \l quitOnLastWindowClosed to false.

    \sa quitOnLastWindowClosed, lastWindowClosed(), QWidget::close(),
    QWidget::closeEvent(), lastWindowClosed(), QCoreApplication::quit(), topLevelWidgets(),
    QWidget::isWindow()
*/
void QApplication::closeAllWindows()
{
    QWindowList processedWindows;
    QApplicationPrivate::tryCloseAllWidgetWindows(&processedWindows);
}

/*!
    Displays a simple message box about Qt. The message includes the version
    number of Qt being used by the application.

    This is useful for inclusion in the \uicontrol Help menu of an application, as
    shown in the \l{mainwindows/menus}{Menus} example.

    This function is a convenience slot for QMessageBox::aboutQt().
*/
void QApplication::aboutQt()
{
#if QT_CONFIG(messagebox)
    QMessageBox::aboutQt(activeWindow());
#endif // QT_CONFIG(messagebox)
}

/*!
    \since 4.1
    \fn void QApplication::focusChanged(QWidget *old, QWidget *now)

    This signal is emitted when the widget that has keyboard focus changed from
    \a old to \a now, i.e., because the user pressed the tab-key, clicked into
    a widget or changed the active window. Both \a old and \a now can be \nullptr.


    The signal is emitted after both widget have been notified about the change
    through QFocusEvent.

    \sa QWidget::setFocus(), QWidget::clearFocus(), Qt::FocusReason
*/

/*!\reimp

*/
bool QApplication::event(QEvent *e)
{
    Q_D(QApplication);
    if (e->type() == QEvent::Quit) {
        closeAllWindows();
        for (auto *w : topLevelWidgets()) {
            if (w->isVisible() && !(w->windowType() == Qt::Desktop) && !(w->windowType() == Qt::Popup) &&
                 (!(w->windowType() == Qt::Dialog) || !w->parentWidget()) && !w->testAttribute(Qt::WA_DontShowOnScreen)) {
                e->ignore();
                return true;
            }
        }
        // Explicitly call QCoreApplication instead of QGuiApplication so that
        // we don't let QGuiApplication close any windows we skipped earlier in
        // closeAllWindows(). FIXME: Unify all this close magic through closeAllWindows.
        return QCoreApplication::event(e);
#ifndef Q_OS_WIN
    } else if (e->type() == QEvent::LocaleChange) {
        // on Windows the event propagation is taken care by the
        // WM_SETTINGCHANGE event handler.
        const QWidgetList list = topLevelWidgets();
        for (auto *w : list) {
            if (!(w->windowType() == Qt::Desktop)) {
                if (!w->testAttribute(Qt::WA_SetLocale))
                    w->d_func()->setLocale_helper(QLocale(), true);
            }
        }
#endif
    } else if (e->type() == QEvent::Timer) {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        Q_ASSERT(te != nullptr);
        if (te->timerId() == d->toolTipWakeUp.timerId()) {
            d->toolTipWakeUp.stop();
            if (d->toolTipWidget) {
                QWidget *w = d->toolTipWidget->window();
                // show tooltip if WA_AlwaysShowToolTips is set, or if
                // any ancestor of d->toolTipWidget is the active
                // window
                bool showToolTip = w->testAttribute(Qt::WA_AlwaysShowToolTips);
                while (w && !showToolTip) {
                    showToolTip = w->isActiveWindow();
                    w = w->parentWidget();
                    w = w ? w->window() : nullptr;
                }
                if (showToolTip) {
                    QHelpEvent e(QEvent::ToolTip, d->toolTipPos, d->toolTipGlobalPos);
                    QCoreApplication::sendEvent(d->toolTipWidget, &e);
                    if (e.isAccepted()) {
                        QStyle *s = d->toolTipWidget->style();
                        int sleepDelay = s->styleHint(QStyle::SH_ToolTip_FallAsleepDelay, nullptr, d->toolTipWidget, nullptr);
                        d->toolTipFallAsleep.start(sleepDelay, this);
                    }
                }
            }
        } else if (te->timerId() == d->toolTipFallAsleep.timerId()) {
            d->toolTipFallAsleep.stop();
        }
#if QT_CONFIG(whatsthis)
    } else if (e->type() == QEvent::EnterWhatsThisMode) {
        QWhatsThis::enterWhatsThisMode();
        return true;
#endif
    }

    if(e->type() == QEvent::LanguageChange) {
        // QGuiApplication::event does not account for the cases where
        // there is a top level widget without a window handle. So they
        // need to have the event posted here
        const QWidgetList list = topLevelWidgets();
        for (auto *w : list) {
            if (!w->windowHandle() && (w->windowType() != Qt::Desktop))
                postEvent(w, new QEvent(QEvent::LanguageChange));
        }
    }

    return QGuiApplication::event(e);
}

// ### FIXME: topLevelWindows does not contain QWidgets without a parent
// until QWidgetPrivate::create is called. So we have to override the
// QGuiApplication::notifyLayoutDirectionChange
// to do the right thing.
void QApplicationPrivate::notifyLayoutDirectionChange()
{
    const QWidgetList list = QApplication::topLevelWidgets();
    QWindowList windowList = QGuiApplication::topLevelWindows();

    // send to all top-level QWidgets
    for (auto *w : list) {
        windowList.removeAll(w->windowHandle());
        QEvent ev(QEvent::ApplicationLayoutDirectionChange);
        QCoreApplication::sendEvent(w, &ev);
    }

    // in case there are any plain QWindows in this QApplication-using
    // application, also send the notification to them
    for (int i = 0; i < windowList.size(); ++i) {
        QEvent ev(QEvent::ApplicationLayoutDirectionChange);
        QCoreApplication::sendEvent(windowList.at(i), &ev);
    }
}

/*!
    \fn void QApplication::setActiveWindow(QWidget* active)

    Sets the active window to the \a active widget in response to a system
    event. The function is called from the platform specific event handlers.

    \warning This function does \e not set the keyboard focus to the active
    widget. Call QWidget::activateWindow() instead.

    It sets the activeWindow() and focusWidget() attributes and sends proper
    \l{QEvent::WindowActivate}{WindowActivate}/\l{QEvent::WindowDeactivate}
    {WindowDeactivate} and \l{QEvent::FocusIn}{FocusIn}/\l{QEvent::FocusOut}
    {FocusOut} events to all appropriate widgets. The window will then be
    painted in active state (e.g. cursors in line edits will blink), and it
    will have tool tips enabled.

    \sa activeWindow(), QWidget::activateWindow()
*/
void QApplication::setActiveWindow(QWidget* act)
{
    QWidget* window = act?act->window():nullptr;

    if (QApplicationPrivate::active_window == window)
        return;

#if QT_CONFIG(graphicsview)
    if (window && window->graphicsProxyWidget()) {
        // Activate the proxy's view->viewport() ?
        return;
    }
#endif

    QWidgetList toBeActivated;
    QWidgetList toBeDeactivated;

    if (QApplicationPrivate::active_window) {
        if (style()->styleHint(QStyle::SH_Widget_ShareActivation, nullptr, QApplicationPrivate::active_window)) {
            const QWidgetList list = topLevelWidgets();
            for (auto *w : list) {
                if (w->isVisible() && w->isActiveWindow())
                    toBeDeactivated.append(w);
            }
        } else {
            toBeDeactivated.append(QApplicationPrivate::active_window);
        }
    }

    if (QApplicationPrivate::focus_widget) {
        if (QApplicationPrivate::focus_widget->testAttribute(Qt::WA_InputMethodEnabled))
            QGuiApplication::inputMethod()->commit();

        QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange, Qt::ActiveWindowFocusReason);
        QCoreApplication::sendEvent(QApplicationPrivate::focus_widget, &focusAboutToChange);
    }

    QApplicationPrivate::active_window = window;

    if (QApplicationPrivate::active_window) {
        if (style()->styleHint(QStyle::SH_Widget_ShareActivation, nullptr, QApplicationPrivate::active_window)) {
            const QWidgetList list = topLevelWidgets();
            for (auto *w : list) {
                if (w->isVisible() && w->isActiveWindow())
                    toBeActivated.append(w);
            }
        } else {
            toBeActivated.append(QApplicationPrivate::active_window);
        }

    }

    // first the activation/deactivation events
    QEvent activationChange(QEvent::ActivationChange);
    QEvent windowActivate(QEvent::WindowActivate);
    QEvent windowDeactivate(QEvent::WindowDeactivate);

    for (int i = 0; i < toBeActivated.size(); ++i) {
        QWidget *w = toBeActivated.at(i);
        sendSpontaneousEvent(w, &windowActivate);
        sendSpontaneousEvent(w, &activationChange);
    }

    for(int i = 0; i < toBeDeactivated.size(); ++i) {
        QWidget *w = toBeDeactivated.at(i);
        sendSpontaneousEvent(w, &windowDeactivate);
        sendSpontaneousEvent(w, &activationChange);
    }

    if (QApplicationPrivate::popupWidgets == nullptr) { // !inPopupMode()
        // then focus events
        if (!QApplicationPrivate::active_window && QApplicationPrivate::focus_widget) {
            QApplicationPrivate::setFocusWidget(nullptr, Qt::ActiveWindowFocusReason);
        } else if (QApplicationPrivate::active_window) {
            QWidget *w = QApplicationPrivate::active_window->focusWidget();
            if (w && w->isVisible() /*&& w->focusPolicy() != QWidget::NoFocus*/)
                w->setFocus(Qt::ActiveWindowFocusReason);
            else {
                w = QApplicationPrivate::focusNextPrevChild_helper(QApplicationPrivate::active_window, true);
                if (w) {
                    w->setFocus(Qt::ActiveWindowFocusReason);
                } else {
                    // If the focus widget is not in the activate_window, clear the focus
                    w = QApplicationPrivate::focus_widget;
                    if (!w && QApplicationPrivate::active_window->focusPolicy() != Qt::NoFocus)
                        QApplicationPrivate::setFocusWidget(QApplicationPrivate::active_window, Qt::ActiveWindowFocusReason);
                    else if (!QApplicationPrivate::active_window->isAncestorOf(w))
                        QApplicationPrivate::setFocusWidget(nullptr, Qt::ActiveWindowFocusReason);
                }
            }
        }
    }
}

QWidget *qt_tlw_for_window(QWindow *wnd)
{
    // QTBUG-32177, wnd might be a QQuickView embedded via window container.
    while (wnd && !wnd->isTopLevel()) {
        QWindow *parent = wnd->parent();
        if (!parent)
            break;

        // Don't end up in windows not belonging to this application
        if (parent->handle() && parent->handle()->isForeignWindow())
            break;

        wnd = wnd->parent();
    }
    if (wnd) {
        const auto tlws = QApplication::topLevelWidgets();
        for (QWidget *tlw : tlws) {
            if (tlw->windowHandle() == wnd)
                return tlw;
        }
    }
    return nullptr;
}

void QApplicationPrivate::notifyActiveWindowChange(QWindow *previous)
{
    Q_UNUSED(previous);
    QWindow *wnd = QGuiApplicationPrivate::focus_window;
    if (inPopupMode()) // some delayed focus event to ignore
        return;
    QWidget *tlw = qt_tlw_for_window(wnd);
    QApplication::setActiveWindow(tlw);
    // QTBUG-37126, Active X controls may set the focus on native child widgets.
    if (wnd && tlw && wnd != tlw->windowHandle()) {
        if (QWidgetWindow *widgetWindow = qobject_cast<QWidgetWindow *>(wnd))
            if (QWidget *widget = widgetWindow->widget())
                if (widget->inherits("QAxHostWidget"))
                    widget->setFocus(Qt::ActiveWindowFocusReason);
    }
}

/*!internal
 * Helper function that returns the new focus widget, but does not set the focus reason.
 * Returns \nullptr if a new focus widget could not be found.
 * Shared with QGraphicsProxyWidgetPrivate::findFocusChild()
*/
QWidget *QApplicationPrivate::focusNextPrevChild_helper(QWidget *toplevel, bool next,
                                                        bool *wrappingOccurred)
{
    uint focus_flag = qt_tab_all_widgets() ? Qt::TabFocus : Qt::StrongFocus;

    QWidget *f = toplevel->focusWidget();
    if (!f)
        f = toplevel;

    QWidget *w = f;
    QWidget *test = f->d_func()->focus_next;
    bool seenWindow = false;
    bool focusWidgetAfterWindow = false;
    while (test && test != f) {
        if (test->isWindow())
            seenWindow = true;

        // If the next focus widget has a focus proxy, we need to check to ensure
        // that the proxy is in the correct parent-child direction (according to
        // \a next). This is to ensure that we can tab in and out of compound widgets
        // without getting stuck in a tab-loop between parent and child.
        QWidget *focusProxy = test->d_func()->deepestFocusProxy();
        const bool canTakeFocus = ((focusProxy ? focusProxy->focusPolicy() : test->focusPolicy())
                                  & focus_flag) == focus_flag;
        const bool composites = focusProxy ? (next ? focusProxy->isAncestorOf(test)
                                                   : test->isAncestorOf(focusProxy))
                                           : false;
        if (canTakeFocus && !composites
            && test->isVisibleTo(toplevel) && test->isEnabled()
            && !(w->windowType() == Qt::SubWindow && !w->isAncestorOf(test))
            && (toplevel->windowType() != Qt::SubWindow || toplevel->isAncestorOf(test))
            && f != focusProxy) {
            w = test;
            if (seenWindow)
                focusWidgetAfterWindow = true;
            if (next)
                break;
        }
        test = test->d_func()->focus_next;
    }

    if (wrappingOccurred != nullptr)
        *wrappingOccurred = next ? focusWidgetAfterWindow : !focusWidgetAfterWindow;

    if (w == f) {
        if (qt_in_tab_key_event) {
            w->window()->setAttribute(Qt::WA_KeyboardFocusChange);
            w->update();
        }
        return nullptr;
    }
    return w;
}

/*!
    \fn void QApplicationPrivate::dispatchEnterLeave(QWidget* enter, QWidget* leave, const QPointF &globalPosF)
    \internal

    Creates the proper Enter/Leave event when widget \a enter is entered and
    widget \a leave is left.
 */
void QApplicationPrivate::dispatchEnterLeave(QWidget* enter, QWidget* leave, const QPointF &globalPosF)
{
#if 0
    if (leave) {
        QEvent e(QEvent::Leave);
        QCoreApplication::sendEvent(leave, & e);
    }
    if (enter) {
        const QPoint windowPos = enter->window()->mapFromGlobal(globalPos);
        QEnterEvent e(enter->mapFromGlobal(globalPos), windowPos, globalPos);
        QCoreApplication::sendEvent(enter, & e);
    }
    return;
#endif

    if ((!enter && !leave) || (enter == leave))
        return;
#ifdef ALIEN_DEBUG
    qDebug() << "QApplicationPrivate::dispatchEnterLeave, ENTER:" << enter << "LEAVE:" << leave;
#endif
    QWidgetList leaveList;
    QWidgetList enterList;

    bool sameWindow = leave && enter && leave->window() == enter->window();
    if (leave && !sameWindow) {
        auto *w = leave;
        do {
            leaveList.append(w);
        } while (!w->isWindow() && (w = w->parentWidget()));
    }
    if (enter && !sameWindow) {
        auto *w = enter;
        do {
            enterList.append(w);
        } while (!w->isWindow() && (w = w->parentWidget()));
    }
    if (sameWindow) {
        int enterDepth = 0;
        int leaveDepth = 0;
        auto *e = enter;
        while (!e->isWindow() && (e = e->parentWidget()))
            enterDepth++;
        auto *l = leave;
        while (!l->isWindow() && (l = l->parentWidget()))
            leaveDepth++;
        QWidget* wenter = enter;
        QWidget* wleave = leave;
        while (enterDepth > leaveDepth) {
            wenter = wenter->parentWidget();
            enterDepth--;
        }
        while (leaveDepth > enterDepth) {
            wleave = wleave->parentWidget();
            leaveDepth--;
        }
        while (!wenter->isWindow() && wenter != wleave) {
            wenter = wenter->parentWidget();
            wleave = wleave->parentWidget();
        }

        for (auto *w = leave; w != wleave; w = w->parentWidget())
            leaveList.append(w);

        for (auto *w = enter; w != wenter; w = w->parentWidget())
            enterList.append(w);
    }

    QEvent leaveEvent(QEvent::Leave);
    for (int i = 0; i < leaveList.size(); ++i) {
        auto *w = leaveList.at(i);
        if (!QApplication::activeModalWidget() || QApplicationPrivate::tryModalHelper(w, nullptr)) {
            QCoreApplication::sendEvent(w, &leaveEvent);
            if (w->testAttribute(Qt::WA_Hover) &&
                (!QApplication::activePopupWidget() || QApplication::activePopupWidget() == w->window())) {
                Q_ASSERT(instance());
                QHoverEvent he(QEvent::HoverLeave, QPoint(-1, -1), w->mapFromGlobal(QApplicationPrivate::instance()->hoverGlobalPos),
                               QGuiApplication::keyboardModifiers());
                qApp->d_func()->notify_helper(w, &he);
            }
        }
    }
    if (!enterList.isEmpty()) {
        // Guard against QGuiApplicationPrivate::lastCursorPosition initialized to qInf(), qInf().
        const QPoint globalPos = qIsInf(globalPosF.x())
            ? QPoint(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)
            : globalPosF.toPoint();
        const QPoint windowPos = qAsConst(enterList).back()->window()->mapFromGlobal(globalPos);
        for (auto it = enterList.crbegin(), end = enterList.crend(); it != end; ++it) {
            auto *w = *it;
            if (!QApplication::activeModalWidget() || QApplicationPrivate::tryModalHelper(w, nullptr)) {
                const QPointF localPos = w->mapFromGlobal(globalPos);
                QEnterEvent enterEvent(localPos, windowPos, globalPosF);
                QCoreApplication::sendEvent(w, &enterEvent);
                if (w->testAttribute(Qt::WA_Hover) &&
                        (!QApplication::activePopupWidget() || QApplication::activePopupWidget() == w->window())) {
                    QHoverEvent he(QEvent::HoverEnter, localPos, QPoint(-1, -1),
                                   QGuiApplication::keyboardModifiers());
                    qApp->d_func()->notify_helper(w, &he);
                }
            }
        }
    }

#ifndef QT_NO_CURSOR
    // Update cursor for alien/graphics widgets.

    const bool enterOnAlien = (enter && (isAlien(enter) || enter->testAttribute(Qt::WA_DontShowOnScreen)));
    // Whenever we leave an alien widget on X11/QPA, we need to reset its nativeParentWidget()'s cursor.
    // This is not required on Windows as the cursor is reset on every single mouse move.
    QWidget *parentOfLeavingCursor = nullptr;
    for (int i = 0; i < leaveList.size(); ++i) {
        auto *w = leaveList.at(i);
        if (!isAlien(w))
            break;
        if (w->testAttribute(Qt::WA_SetCursor)) {
            QWidget *parent = w->parentWidget();
            while (parent && parent->d_func()->data.in_destructor)
                parent = parent->parentWidget();
            parentOfLeavingCursor = parent;
            //continue looping, we need to find the downest alien widget with a cursor.
            // (downest on the screen)
        }
    }
    //check that we will not call qt_x11_enforce_cursor twice with the same native widget
    if (parentOfLeavingCursor && (!enterOnAlien
        || parentOfLeavingCursor->effectiveWinId() != enter->effectiveWinId())) {
#if QT_CONFIG(graphicsview)
        if (!parentOfLeavingCursor->window()->graphicsProxyWidget())
#endif
        {
            if (enter == QApplication::desktop()) {
                qt_qpa_set_cursor(enter, true);
            } else {
                qt_qpa_set_cursor(parentOfLeavingCursor, true);
            }
        }
    }
    if (enterOnAlien) {
        QWidget *cursorWidget = enter;
        while (!cursorWidget->isWindow() && !cursorWidget->isEnabled())
            cursorWidget = cursorWidget->parentWidget();

        if (!cursorWidget)
            return;

#if QT_CONFIG(graphicsview)
        if (cursorWidget->window()->graphicsProxyWidget()) {
            QWidgetPrivate::nearestGraphicsProxyWidget(cursorWidget)->setCursor(cursorWidget->cursor());
        } else
#endif
        {
            qt_qpa_set_cursor(cursorWidget, true);
        }
    }
#endif
}

/* exported for the benefit of testing tools */
Q_WIDGETS_EXPORT bool qt_tryModalHelper(QWidget *widget, QWidget **rettop)
{
    return QApplicationPrivate::tryModalHelper(widget, rettop);
}

/*! \internal
    Returns \c true if \a widget is blocked by a modal window.
 */
bool QApplicationPrivate::isBlockedByModal(QWidget *widget)
{
    widget = widget->window();
    QWindow *window = widget->windowHandle();
    return window && self->isWindowBlocked(window);
}

bool QApplicationPrivate::isWindowBlocked(QWindow *window, QWindow **blockingWindow) const
{
    QWindow *unused = nullptr;
    if (Q_UNLIKELY(!window)) {
        qWarning().nospace() << "window == 0 passed.";
        return false;
    }
    if (!blockingWindow)
        blockingWindow = &unused;

    if (modalWindowList.isEmpty()) {
        *blockingWindow = nullptr;
        return false;
    }
    QWidget *popupWidget = QApplication::activePopupWidget();
    QWindow *popupWindow = popupWidget ? popupWidget->windowHandle() : nullptr;
    if (popupWindow == window || (!popupWindow && QWindowPrivate::get(window)->isPopup())) {
        *blockingWindow = nullptr;
        return false;
    }

    for (int i = 0; i < modalWindowList.count(); ++i) {
        QWindow *modalWindow = modalWindowList.at(i);

        // A window is not blocked by another modal window if the two are
        // the same, or if the window is a child of the modal window.
        if (window == modalWindow || modalWindow->isAncestorOf(window, QWindow::IncludeTransients)) {
            *blockingWindow = nullptr;
            return false;
        }

        Qt::WindowModality windowModality = modalWindow->modality();
        QWidgetWindow *modalWidgetWindow = qobject_cast<QWidgetWindow *>(modalWindow);
        if (windowModality == Qt::NonModal) {
            // determine the modality type if it hasn't been set on the
            // modalWindow's widget, this normally happens when waiting for a
            // native dialog. use WindowModal if we are the child of a group
            // leader; otherwise use ApplicationModal.
            QWidget *m = modalWidgetWindow ? modalWidgetWindow->widget() : nullptr;
            while (m && !m->testAttribute(Qt::WA_GroupLeader)) {
                m = m->parentWidget();
                if (m)
                    m = m->window();
            }
            windowModality = (m && m->testAttribute(Qt::WA_GroupLeader))
                             ? Qt::WindowModal
                             : Qt::ApplicationModal;
        }

        switch (windowModality) {
        case Qt::ApplicationModal:
        {
            QWidgetWindow *widgetWindow = qobject_cast<QWidgetWindow *>(window);
            QWidget *groupLeaderForWidget = widgetWindow ? widgetWindow->widget() : nullptr;
            while (groupLeaderForWidget && !groupLeaderForWidget->testAttribute(Qt::WA_GroupLeader))
                groupLeaderForWidget = groupLeaderForWidget->parentWidget();

            if (groupLeaderForWidget) {
                // if \a widget has WA_GroupLeader, it can only be blocked by ApplicationModal children
                QWidget *m = modalWidgetWindow ? modalWidgetWindow->widget() : nullptr;
                while (m && m != groupLeaderForWidget && !m->testAttribute(Qt::WA_GroupLeader))
                    m = m->parentWidget();
                if (m == groupLeaderForWidget) {
                    *blockingWindow = m->windowHandle();
                    return true;
                }
            } else if (modalWindow != window) {
                *blockingWindow = modalWindow;
                return true;
            }
            break;
        }
        case Qt::WindowModal:
        {
            QWindow *w = window;
            do {
                QWindow *m = modalWindow;
                do {
                    if (m == w) {
                        *blockingWindow = m;
                        return true;
                    }
                    QWindow *p = m->parent();
                    if (!p)
                        p = m->transientParent();
                    m = p;
                } while (m);
                QWindow *p = w->parent();
                if (!p)
                    p = w->transientParent();
                w = p;
            } while (w);
            break;
        }
        default:
            Q_ASSERT_X(false, "QApplication", "internal error, a modal window cannot be modeless");
            break;
        }
    }
    *blockingWindow = nullptr;
    return false;
}

/*!\internal

  Called from qapplication_\e{platform}.cpp, returns \c true
  if the widget should accept the event.
 */
bool QApplicationPrivate::tryModalHelper(QWidget *widget, QWidget **rettop)
{
    QWidget *top = QApplication::activeModalWidget();
    if (rettop)
        *rettop = top;

    // the active popup widget always gets the input event
    if (QApplication::activePopupWidget())
        return true;

    return !isBlockedByModal(widget->window());
}

bool qt_try_modal(QWidget *widget, QEvent::Type type)
{
    QWidget * top = nullptr;

    if (QApplicationPrivate::tryModalHelper(widget, &top))
        return true;

    bool block_event  = false;

    switch (type) {
#if 0
    case QEvent::Focus:
        if (!static_cast<QWSFocusEvent*>(event)->simpleData.get_focus)
            break;
        // drop through
#endif
    case QEvent::MouseButtonPress:                        // disallow mouse/key events
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        block_event         = true;
        break;
    default:
        break;
    }

    if (block_event && top && top->parentWidget() == nullptr)
        top->raise();

    return !block_event;
}

bool QApplicationPrivate::modalState()
{
    return !self->modalWindowList.isEmpty();
}

/*
   \internal
*/
QWidget *QApplicationPrivate::pickMouseReceiver(QWidget *candidate, const QPoint &windowPos,
                                                QPoint *pos, QEvent::Type type,
                                                Qt::MouseButtons buttons, QWidget *buttonDown,
                                                QWidget *alienWidget)
{
    Q_ASSERT(candidate);

    QWidget *mouseGrabber = QWidget::mouseGrabber();
    if (((type == QEvent::MouseMove && buttons) || (type == QEvent::MouseButtonRelease))
            && !buttonDown && !mouseGrabber) {
        return nullptr;
    }

    if (alienWidget && alienWidget->internalWinId())
        alienWidget = nullptr;

    QWidget *receiver = candidate;

    if (!mouseGrabber)
        mouseGrabber = (buttonDown && !isBlockedByModal(buttonDown)) ? buttonDown : alienWidget;

    if (mouseGrabber && mouseGrabber != candidate) {
        receiver = mouseGrabber;
        *pos = receiver->mapFromGlobal(candidate->mapToGlobal(windowPos));
#ifdef ALIEN_DEBUG
        qDebug() << "  ** receiver adjusted to:" << receiver << "pos:" << pos;
#endif
    }

    return receiver;

}

/*
   \internal
*/
bool QApplicationPrivate::sendMouseEvent(QWidget *receiver, QMouseEvent *event,
                                         QWidget *alienWidget, QWidget *nativeWidget,
                                         QWidget **buttonDown, QPointer<QWidget> &lastMouseReceiver,
                                         bool spontaneous, bool onlyDispatchEnterLeave)
{
    Q_ASSERT(receiver);
    Q_ASSERT(event);
    Q_ASSERT(nativeWidget);
    Q_ASSERT(buttonDown);

    if (alienWidget && !isAlien(alienWidget))
        alienWidget = nullptr;

    QPointer<QWidget> receiverGuard = receiver;
    QPointer<QWidget> nativeGuard = nativeWidget;
    QPointer<QWidget> alienGuard = alienWidget;
    QPointer<QWidget> activePopupWidget = QApplication::activePopupWidget();

    const bool graphicsWidget = nativeWidget->testAttribute(Qt::WA_DontShowOnScreen);

    bool widgetUnderMouse = QRectF(receiver->rect()).contains(event->localPos());

    // Clear the obsolete leaveAfterRelease value, if mouse button has been released but
    // leaveAfterRelease has not been updated.
    // This happens e.g. when modal dialog or popup is shown as a response to button click.
    if (leaveAfterRelease && !*buttonDown && !event->buttons())
        leaveAfterRelease = nullptr;

    if (*buttonDown) {
        if (!graphicsWidget) {
            // Register the widget that shall receive a leave event
            // after the last button is released.
            if ((alienWidget || !receiver->internalWinId()) && !leaveAfterRelease && !QWidget::mouseGrabber())
                leaveAfterRelease = *buttonDown;
            if (event->type() == QEvent::MouseButtonRelease && !event->buttons())
                *buttonDown = nullptr;
        }
    } else if (lastMouseReceiver && widgetUnderMouse) {
        // Dispatch enter/leave if we move:
        // 1) from an alien widget to another alien widget or
        //    from a native widget to an alien widget (first OR case)
        // 2) from an alien widget to a native widget (second OR case)
        if ((alienWidget && alienWidget != lastMouseReceiver)
            || (isAlien(lastMouseReceiver) && !alienWidget)) {
            if (activePopupWidget) {
                if (!QWidget::mouseGrabber())
                    dispatchEnterLeave(alienWidget ? alienWidget : nativeWidget, lastMouseReceiver, event->screenPos());
            } else {
                dispatchEnterLeave(receiver, lastMouseReceiver, event->screenPos());
            }

        }
    }

#ifdef ALIEN_DEBUG
    qDebug() << "QApplicationPrivate::sendMouseEvent: receiver:" << receiver
             << "pos:" << event->pos() << "alien" << alienWidget << "button down"
             << *buttonDown << "last" << lastMouseReceiver << "leave after release"
             << leaveAfterRelease;
#endif

    // We need this quard in case someone opens a modal dialog / popup. If that's the case
    // leaveAfterRelease is set to null, but we shall not update lastMouseReceiver.
    const bool wasLeaveAfterRelease = leaveAfterRelease != nullptr;
    bool result = true;
    // This code is used for sending the synthetic enter/leave events for cases where it is needed
    // due to other events causing the widget under the mouse to change. However in those cases
    // we do not want to send the mouse event associated with this call, so this enables us to
    // not send the unneeded mouse event
    if (!onlyDispatchEnterLeave) {
        if (spontaneous)
            result = QApplication::sendSpontaneousEvent(receiver, event);
        else
            result = QCoreApplication::sendEvent(receiver, event);
    }

    if (!graphicsWidget && leaveAfterRelease && event->type() == QEvent::MouseButtonRelease
        && !event->buttons() && QWidget::mouseGrabber() != leaveAfterRelease) {
        // Dispatch enter/leave if:
        // 1) the mouse grabber is an alien widget
        // 2) the button is released on an alien widget
        QWidget *enter = nullptr;
        if (nativeGuard)
            enter = alienGuard ? alienWidget : nativeWidget;
        else // The receiver is typically deleted on mouse release with drag'n'drop.
            enter = QApplication::widgetAt(event->globalPos());
        dispatchEnterLeave(enter, leaveAfterRelease, event->screenPos());
        leaveAfterRelease = nullptr;
        lastMouseReceiver = enter;
    } else if (!wasLeaveAfterRelease) {
        if (activePopupWidget) {
            if (!QWidget::mouseGrabber())
                lastMouseReceiver = alienGuard ? alienWidget : (nativeGuard ? nativeWidget : nullptr);
        } else {
            lastMouseReceiver = receiverGuard ? receiver : QApplication::widgetAt(event->globalPos());
        }
    }

    return result;
}

/*
    This function should only be called when the widget changes visibility, i.e.
    when the \a widget is shown, hidden or deleted. This function does nothing
    if the widget is a top-level or native, i.e. not an alien widget. In that
    case enter/leave events are genereated by the underlying windowing system.
*/
extern QPointer<QWidget> qt_last_mouse_receiver;
extern Q_WIDGETS_EXPORT QWidget *qt_button_down;
void QApplicationPrivate::sendSyntheticEnterLeave(QWidget *widget)
{
#ifndef QT_NO_CURSOR
    if (!widget || widget->isWindow())
        return;
    const bool widgetInShow = widget->isVisible() && !widget->data->in_destructor;
    if (!widgetInShow && widget != qt_last_mouse_receiver)
        return; // Widget was not under the cursor when it was hidden/deleted.

    if (widgetInShow && widget->parentWidget()->data->in_show)
        return; // Ingore recursive show.

    QWidget *mouseGrabber = QWidget::mouseGrabber();
    if (mouseGrabber && mouseGrabber != widget)
        return; // Someone else has the grab; enter/leave should not occur.

    QWidget *tlw = widget->window();
    if (tlw->data->in_destructor || tlw->data->is_closing)
        return; // Closing down the business.

    if (widgetInShow && (!qt_last_mouse_receiver || qt_last_mouse_receiver->window() != tlw))
        return; // Mouse cursor not inside the widget's top-level.

    const QPoint globalPos(QCursor::pos());
    QPoint windowPos = tlw->mapFromGlobal(globalPos);

    // Find the current widget under the mouse. If this function was called from
    // the widget's destructor, we have to make sure childAt() doesn't take into
    // account widgets that are about to be destructed.
    QWidget *widgetUnderCursor = tlw->d_func()->childAt_helper(windowPos, widget->data->in_destructor);
    if (!widgetUnderCursor)
        widgetUnderCursor = tlw;
    QPoint pos = widgetUnderCursor->mapFrom(tlw, windowPos);

    if (widgetInShow && widgetUnderCursor != widget && !widget->isAncestorOf(widgetUnderCursor))
        return; // Mouse cursor not inside the widget or any of its children.

    if (widget->data->in_destructor && qt_button_down == widget)
        qt_button_down = nullptr;

    // A mouse move is not actually sent, but we utilize the sendMouseEvent() call to send the
    // enter/leave events as appropriate
    QMouseEvent e(QEvent::MouseMove, pos, windowPos, globalPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    sendMouseEvent(widgetUnderCursor, &e, widgetUnderCursor, tlw, &qt_button_down, qt_last_mouse_receiver, true, true);
#else // !QT_NO_CURSOR
    Q_UNUSED(widget);
#endif // QT_NO_CURSOR
}

/*!
    \obsolete

    Returns the desktop widget (also called the root window).

    The desktop may be composed of multiple screens, so it would be incorrect,
    for example, to attempt to \e center some widget in the desktop's geometry.
    QDesktopWidget has various functions for obtaining useful geometries upon
    the desktop, such as QDesktopWidget::screenGeometry() and
    QDesktopWidget::availableGeometry().
*/
QDesktopWidget *QApplication::desktop()
{
    CHECK_QAPP_INSTANCE(nullptr)
    if (!qt_desktopWidget || // not created yet
         !(qt_desktopWidget->windowType() == Qt::Desktop)) { // reparented away
        qt_desktopWidget = new QDesktopWidget();
    }
    return qt_desktopWidget;
}

/*
  Sets the time after which a drag should start to \a ms ms.

  \sa startDragTime()
*/

void QApplication::setStartDragTime(int ms)
{
    QGuiApplication::styleHints()->setStartDragTime(ms);
}

/*!
    \property QApplication::startDragTime
    \brief the time in milliseconds that a mouse button must be held down
    before a drag and drop operation will begin

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has held down a mouse button for a
    certain amount of time, you should use this property's value as the delay.

    Qt also uses this delay internally, e.g. in QTextEdit and QLineEdit, for
    starting a drag.

    The default value is 500 ms.

    \sa startDragDistance(), {Drag and Drop}
*/

int QApplication::startDragTime()
{
    return QGuiApplication::styleHints()->startDragTime();
}

/*
    Sets the distance after which a drag should start to \a l pixels.

    \sa startDragDistance()
*/

void QApplication::setStartDragDistance(int l)
{
    QGuiApplication::styleHints()->setStartDragDistance(l);
}

/*!
    \property QApplication::startDragDistance

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has moved the cursor a certain distance
    with a button held down, you should use this property's value as the
    minimum distance required.

    For example, if the mouse position of the click is stored in \c startPos
    and the current position (e.g. in the mouse move event) is \c currentPos,
    you can find out if a drag should be started with code like this:

    \snippet code/src_gui_kernel_qapplication.cpp 7

    Qt uses this value internally, e.g. in QFileDialog.

    The default value (if the platform doesn't provide a different default)
    is 10 pixels.

    \sa startDragTime(), QPoint::manhattanLength(), {Drag and Drop}
*/

int QApplication::startDragDistance()
{
    return QGuiApplication::styleHints()->startDragDistance();
}

/*!
    Enters the main event loop and waits until exit() is called, then returns
    the value that was set to exit() (which is 0 if exit() is called via
    quit()).

    It is necessary to call this function to start event handling. The main
    event loop receives events from the window system and dispatches these to
    the application widgets.

    Generally, no user interaction can take place before calling exec(). As a
    special case, modal widgets like QMessageBox can be used before calling
    exec(), because modal widgets call exec() to start a local event loop.

    To make your application perform idle processing, i.e., executing a special
    function whenever there are no pending events, use a QTimer with 0 timeout.
    More advanced idle processing schemes can be achieved using processEvents().

    We recommend that you connect clean-up code to the
    \l{QCoreApplication::}{aboutToQuit()} signal, instead of putting it in your
    application's \c{main()} function. This is because, on some platforms the
    QApplication::exec() call may not return. For example, on the Windows
    platform, when the user logs off, the system terminates the process after Qt
    closes all top-level windows. Hence, there is \e{no guarantee} that the
    application will have time to exit its event loop and execute code at the
    end of the \c{main()} function, after the QApplication::exec() call.

    \sa quitOnLastWindowClosed, QCoreApplication::quit(), QCoreApplication::exit(),
        QCoreApplication::processEvents(), QCoreApplication::exec()
*/
int QApplication::exec()
{
    return QGuiApplication::exec();
}

bool QApplicationPrivate::shouldQuit()
{
    /* if there is no non-withdrawn primary window left (except
        the ones without QuitOnClose), we emit the lastWindowClosed
        signal */
    QWidgetList list = QApplication::topLevelWidgets();
    QWindowList processedWindows;
    for (int i = 0; i < list.size(); ++i) {
        QWidget *w = list.at(i);
        if (QWindow *window = w->windowHandle()) { // Menus, popup widgets may not have a QWindow
            processedWindows.push_back(window);
            if (w->isVisible() && !w->parentWidget() && w->testAttribute(Qt::WA_QuitOnClose))
                return false;
        }
    }
    return QGuiApplicationPrivate::shouldQuitInternal(processedWindows);
}

static inline void closeAllPopups()
{
    // Close all popups: In case some popup refuses to close,
    // we give up after 1024 attempts (to avoid an infinite loop).
    int maxiter = 1024;
    QWidget *popup;
    while ((popup = QApplication::activePopupWidget()) && maxiter--)
        popup->close();
}

/*! \reimp
 */
bool QApplication::notify(QObject *receiver, QEvent *e)
{
    Q_D(QApplication);
    // no events are delivered after ~QCoreApplication() has started
    if (QApplicationPrivate::is_app_closing)
        return true;

    if (Q_UNLIKELY(!receiver)) {                        // serious error
        qWarning("QApplication::notify: Unexpected null receiver");
        return true;
    }

#ifndef QT_NO_DEBUG
    QCoreApplicationPrivate::checkReceiverThread(receiver);
#endif

    if (receiver->isWindowType()) {
        if (QGuiApplicationPrivate::sendQWindowEventToQPlatformWindow(static_cast<QWindow *>(receiver), e))
            return true; // Platform plugin ate the event
    }

    QGuiApplicationPrivate::captureGlobalModifierState(e);

#ifndef QT_NO_GESTURES
    // walk through parents and check for gestures
    if (d->gestureManager) {
        switch (e->type()) {
        case QEvent::Paint:
        case QEvent::MetaCall:
        case QEvent::DeferredDelete:
        case QEvent::DragEnter: case QEvent::DragMove: case QEvent::DragLeave:
        case QEvent::Drop: case QEvent::DragResponse:
        case QEvent::ChildAdded: case QEvent::ChildPolished:
        case QEvent::ChildRemoved:
        case QEvent::UpdateRequest:
        case QEvent::UpdateLater:
        case QEvent::LocaleChange:
        case QEvent::Style:
        case QEvent::IconDrag:
        case QEvent::StyleChange:
        case QEvent::GraphicsSceneDragEnter:
        case QEvent::GraphicsSceneDragMove:
        case QEvent::GraphicsSceneDragLeave:
        case QEvent::GraphicsSceneDrop:
        case QEvent::DynamicPropertyChange:
        case QEvent::NetworkReplyUpdated:
            break;
        default:
            if (d->gestureManager->thread() == QThread::currentThread()) {
                if (receiver->isWidgetType()) {
                    if (d->gestureManager->filterEvent(static_cast<QWidget *>(receiver), e))
                        return true;
                } else {
                    // a special case for events that go to QGesture objects.
                    // We pass the object to the gesture manager and it'll figure
                    // out if it's QGesture or not.
                    if (d->gestureManager->filterEvent(receiver, e))
                        return true;
                }
            }
            break;
        }
    }
#endif // QT_NO_GESTURES

    switch (e->type()) {
    case QEvent::ApplicationDeactivate:
        // Close all popups (triggers when switching applications
        // by pressing ALT-TAB on Windows, which is not receive as key event.
        closeAllPopups();
        break;
    case QEvent::Wheel: // User input and window activation makes tooltips sleep
    case QEvent::ActivationChange:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::FocusOut:
    case QEvent::FocusIn:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        d->toolTipFallAsleep.stop();
        Q_FALLTHROUGH();
    case QEvent::Leave:
        d->toolTipWakeUp.stop();
    default:
        break;
    }

    switch (e->type()) {
        case QEvent::KeyPress: {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            const int key = keyEvent->key();
            // When a key press is received which is not spontaneous then it needs to
            // be manually sent as a shortcut override event to ensure that any
            // matching shortcut is triggered first. This enables emulation/playback
            // of recorded events to still have the same effect.
            if (!e->spontaneous() && receiver->isWidgetType()) {
                if (qt_sendShortcutOverrideEvent(qobject_cast<QWidget *>(receiver), keyEvent->timestamp(),
                                                 key, keyEvent->modifiers(), keyEvent->text(),
                                                 keyEvent->isAutoRepeat(), keyEvent->count()))
                    return true;
            }
            qt_in_tab_key_event = (key == Qt::Key_Backtab
                        || key == Qt::Key_Tab
                        || key == Qt::Key_Left
                        || key == Qt::Key_Up
                        || key == Qt::Key_Right
                        || key == Qt::Key_Down);
        }
        default:
            break;
    }

    bool res = false;
    if (!receiver->isWidgetType()) {
        res = d->notify_helper(receiver, e);
    } else switch (e->type()) {
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        {
            bool isWidget = receiver->isWidgetType();
#if QT_CONFIG(graphicsview)
            const bool isGraphicsWidget = !isWidget && qobject_cast<QGraphicsWidget *>(receiver);
#endif
            QKeyEvent* key = static_cast<QKeyEvent*>(e);
            bool def = key->isAccepted();
            QPointer<QObject> pr = receiver;
            while (receiver) {
                if (def)
                    key->accept();
                else
                    key->ignore();
                QWidget *w = isWidget ? static_cast<QWidget *>(receiver) : nullptr;
#if QT_CONFIG(graphicsview)
                QGraphicsWidget *gw = isGraphicsWidget ? static_cast<QGraphicsWidget *>(receiver) : nullptr;
#endif
                res = d->notify_helper(receiver, e);

                if ((res && key->isAccepted())
                    /*
                       QLineEdit will emit a signal on Key_Return, but
                       ignore the event, and sometimes the connected
                       slot deletes the QLineEdit (common in itemview
                       delegates), so we have to check if the widget
                       was destroyed even if the event was ignored (to
                       prevent a crash)

                       note that we don't have to reset pw while
                       propagating (because the original receiver will
                       be destroyed if one of its ancestors is)
                    */
                    || !pr
                    || (isWidget && (w->isWindow() || !w->parentWidget()))
#if QT_CONFIG(graphicsview)
                    || (isGraphicsWidget && (gw->isWindow() || !gw->parentWidget()))
#endif
                    ) {
                    break;
                }

#if QT_CONFIG(graphicsview)
                receiver = w ? (QObject *)w->parentWidget() : (QObject *)gw->parentWidget();
#else
                receiver = w->parentWidget();
#endif
            }
            qt_in_tab_key_event = false;
        }
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        {
            QWidget* w = static_cast<QWidget *>(receiver);

            QMouseEvent* mouse = static_cast<QMouseEvent*>(e);
            QPoint relpos = mouse->pos();

            if (e->spontaneous()) {
                if (e->type() != QEvent::MouseMove)
                    QApplicationPrivate::giveFocusAccordingToFocusPolicy(w, e, relpos);

                // ### Qt 5 These dynamic tool tips should be an OPT-IN feature. Some platforms
                // like OS X (probably others too), can optimize their views by not
                // dispatching mouse move events. We have attributes to control hover,
                // and mouse tracking, but as long as we are deciding to implement this
                // feature without choice of opting-in or out, you ALWAYS have to have
                // tracking enabled. Therefore, the other properties give a false sense of
                // performance enhancement.
                if (e->type() == QEvent::MouseMove && mouse->buttons() == 0
                    && w->rect().contains(relpos)) { // Outside due to mouse grab?
                    d->toolTipWidget = w;
                    d->toolTipPos = relpos;
                    d->toolTipGlobalPos = mouse->globalPos();
                    QStyle *s = d->toolTipWidget->style();
                    int wakeDelay = s->styleHint(QStyle::SH_ToolTip_WakeUpDelay, nullptr, d->toolTipWidget, nullptr);
                    d->toolTipWakeUp.start(d->toolTipFallAsleep.isActive() ? 20 : wakeDelay, this);
                }
            }

            bool eventAccepted = mouse->isAccepted();

            QPointer<QWidget> pw = w;
            while (w) {
                QMouseEvent me(mouse->type(), relpos, mouse->windowPos(), mouse->globalPos(),
                               mouse->button(), mouse->buttons(), mouse->modifiers(), mouse->source());
                me.spont = mouse->spontaneous();
                me.setTimestamp(mouse->timestamp());
                QGuiApplicationPrivate::setMouseEventFlags(&me, mouse->flags());
                // throw away any mouse-tracking-only mouse events
                if (!w->hasMouseTracking()
                    && mouse->type() == QEvent::MouseMove && mouse->buttons() == 0) {
                    // but still send them through all application event filters (normally done by notify_helper)
                    d->sendThroughApplicationEventFilters(w, w == receiver ? mouse : &me);
                    res = true;
                } else {
                    w->setAttribute(Qt::WA_NoMouseReplay, false);
                    res = d->notify_helper(w, w == receiver ? mouse : &me);
                    e->spont = false;
                }
                eventAccepted = (w == receiver ? mouse : &me)->isAccepted();
                if (res && eventAccepted)
                    break;
                if (w->isWindow() || w->testAttribute(Qt::WA_NoMousePropagation))
                    break;
                relpos += w->pos();
                w = w->parentWidget();
            }

            mouse->setAccepted(eventAccepted);

            if (e->type() == QEvent::MouseMove) {
                if (!pw)
                    break;

                w = static_cast<QWidget *>(receiver);
                relpos = mouse->pos();
                QPoint diff = relpos - w->mapFromGlobal(d->hoverGlobalPos);
                while (w) {
                    if (w->testAttribute(Qt::WA_Hover) &&
                        (!QApplication::activePopupWidget() || QApplication::activePopupWidget() == w->window())) {
                        QHoverEvent he(QEvent::HoverMove, relpos, relpos - diff, mouse->modifiers());
                        d->notify_helper(w, &he);
                    }
                    if (w->isWindow() || w->testAttribute(Qt::WA_NoMousePropagation))
                        break;
                    relpos += w->pos();
                    w = w->parentWidget();
                }
            }

            d->hoverGlobalPos = mouse->globalPos();
        }
        break;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        {
            QWidget* w = static_cast<QWidget *>(receiver);

            // QTBUG-40656, QTBUG-42731: ignore wheel events when a popup (QComboBox) is open.
            if (const QWidget *popup = QApplication::activePopupWidget()) {
                if (w->window() != popup)
                    return true;
            }

            QWheelEvent* wheel = static_cast<QWheelEvent*>(e);
            const bool spontaneous = wheel->spontaneous();
            const Qt::ScrollPhase phase = wheel->phase();

            // Ideally, we should lock on a widget when it starts receiving wheel
            // events. This avoids other widgets to start receiving those events
            // as the mouse cursor hovers them. However, given the way common
            // wheeled mice work, there's no certain way of connecting different
            // wheel events as a stream. This results in the NoScrollPhase case,
            // where we just send the event from the original receiver and up its
            // hierarchy until the event gets accepted.
            //
            // In the case of more evolved input devices, like Apple's trackpad or
            // Magic Mouse, we receive the scroll phase information. This helps us
            // connect wheel events as a stream and therefore makes it easier to
            // lock on the widget onto which the scrolling was initiated.
            //
            // We assume that, when supported, the phase cycle follows the pattern:
            //
            //         ScrollBegin (ScrollUpdate* ScrollMomentum* ScrollEnd)+
            //
            // This means that we can have scrolling sequences (starting with ScrollBegin)
            // or partial sequences (after a ScrollEnd and starting with ScrollUpdate).
            // If wheel_widget is null because it was deleted, we also take the same
            // code path as an initial sequence.
            if (!spontaneous) {
                // wheel_widget may forward the wheel event to a delegate widget,
                // either directly or indirectly (e.g. QAbstractScrollArea will
                // forward to its QScrollBars through viewportEvent()). In that
                // case, the event will not be spontaneous but synthesized, so
                // we can send it straight to the receiver.
                wheel->ignore();
                res = d->notify_helper(w, wheel);
            } else if (phase == Qt::NoScrollPhase || phase == Qt::ScrollBegin || !QApplicationPrivate::wheel_widget) {
                // A system-generated ScrollBegin event starts a new user scrolling
                // sequence, so we reset wheel_widget in case no one accepts the event
                // or if we didn't get (or missed) a ScrollEnd previously.
                if (phase == Qt::ScrollBegin)
                    QApplicationPrivate::wheel_widget = nullptr;

                const QPoint relpos = wheel->position().toPoint();

                if (phase == Qt::NoScrollPhase || phase == Qt::ScrollUpdate)
                    QApplicationPrivate::giveFocusAccordingToFocusPolicy(w, e, relpos);

#if QT_DEPRECATED_SINCE(5, 14)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
                QWheelEvent we(relpos, wheel->globalPos(), wheel->pixelDelta(), wheel->angleDelta(), wheel->delta(), wheel->orientation(), wheel->buttons(),
                               wheel->modifiers(), phase, wheel->source(), wheel->inverted());
QT_WARNING_POP
#else
                QWheelEvent we(relpos, wheel->globalPosition(), wheel->pixelDelta(), wheel->angleDelta(), wheel->buttons(),
                               wheel->modifiers(), phase, wheel->inverted(), wheel->source());
#endif
                we.setTimestamp(wheel->timestamp());
                bool eventAccepted;
                do {
                    we.spont = w == receiver;
                    we.ignore();
                    res = d->notify_helper(w, &we);
                    eventAccepted = we.isAccepted();
                    if (res && eventAccepted) {
                        // A new scrolling sequence or partial sequence starts and w has accepted
                        // the event. Therefore, we can set wheel_widget, but only if it's not
                        // the end of a sequence.
                        if (QApplicationPrivate::wheel_widget == nullptr && (phase == Qt::ScrollBegin || phase == Qt::ScrollUpdate))
                            QApplicationPrivate::wheel_widget = w;
                        break;
                    }
                    if (w->isWindow() || w->testAttribute(Qt::WA_NoMousePropagation))
                        break;

                    we.p += w->pos();
                    w = w->parentWidget();
                } while (w);
                wheel->setAccepted(eventAccepted);
            } else {
                // The phase is either ScrollUpdate, ScrollMomentum, or ScrollEnd, and wheel_widget
                // is set. Since it accepted the wheel event previously, we continue
                // sending those events until we get a ScrollEnd, which signifies
                // the end of the natural scrolling sequence.
                const QPoint &relpos = QApplicationPrivate::wheel_widget->mapFromGlobal(wheel->globalPosition().toPoint());
#if QT_DEPRECATED_SINCE(5, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
                QWheelEvent we(relpos, wheel->globalPos(), wheel->pixelDelta(), wheel->angleDelta(), wheel->delta(), wheel->orientation(), wheel->buttons(),
                               wheel->modifiers(), wheel->phase(), wheel->source());
QT_WARNING_POP
#else
                QWheelEvent we(relpos, wheel->globalPosition(), wheel->pixelDelta(), wheel->angleDelta(), wheel->buttons(),
                               wheel->modifiers(), wheel->phase(), wheel->inverted(), wheel->source());
#endif
                we.setTimestamp(wheel->timestamp());
                we.spont = true;
                we.ignore();
                d->notify_helper(QApplicationPrivate::wheel_widget, &we);
                wheel->setAccepted(we.isAccepted());
                if (phase == Qt::ScrollEnd)
                    QApplicationPrivate::wheel_widget = nullptr;
            }
        }
        break;
#endif
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        {
            QWidget* w = static_cast<QWidget *>(receiver);
            QContextMenuEvent *context = static_cast<QContextMenuEvent*>(e);
            QPoint relpos = context->pos();
            bool eventAccepted = context->isAccepted();
            while (w) {
                QContextMenuEvent ce(context->reason(), relpos, context->globalPos(), context->modifiers());
                ce.spont = e->spontaneous();
                res = d->notify_helper(w, w == receiver ? context : &ce);
                eventAccepted = ((w == receiver) ? context : &ce)->isAccepted();
                e->spont = false;

                if ((res && eventAccepted)
                    || w->isWindow() || w->testAttribute(Qt::WA_NoMousePropagation))
                    break;

                relpos += w->pos();
                w = w->parentWidget();
            }
            context->setAccepted(eventAccepted);
        }
        break;
#endif // QT_NO_CONTEXTMENU
#if QT_CONFIG(tabletevent)
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        {
            QWidget *w = static_cast<QWidget *>(receiver);
            QTabletEvent *tablet = static_cast<QTabletEvent*>(e);
            QPointF relpos = tablet->posF();
            bool eventAccepted = tablet->isAccepted();
            while (w) {
                QTabletEvent te(tablet->type(), relpos, tablet->globalPosF(),
                                tablet->deviceType(), tablet->pointerType(),
                                tablet->pressure(), tablet->xTilt(), tablet->yTilt(),
                                tablet->tangentialPressure(), tablet->rotation(), tablet->z(),
                                tablet->modifiers(), tablet->uniqueId(), tablet->button(), tablet->buttons());
                te.spont = e->spontaneous();
                te.setAccepted(false);
                res = d->notify_helper(w, w == receiver ? tablet : &te);
                eventAccepted = ((w == receiver) ? tablet : &te)->isAccepted();
                e->spont = false;
                if ((res && eventAccepted)
                     || w->isWindow()
                     || w->testAttribute(Qt::WA_NoMousePropagation))
                    break;

                relpos += w->pos();
                w = w->parentWidget();
            }
            tablet->setAccepted(eventAccepted);
        }
        break;
#endif // QT_CONFIG(tabletevent)

#if !defined(QT_NO_TOOLTIP) || QT_CONFIG(whatsthis)
    case QEvent::ToolTip:
    case QEvent::WhatsThis:
    case QEvent::QueryWhatsThis:
        {
            QWidget* w = static_cast<QWidget *>(receiver);
            QHelpEvent *help = static_cast<QHelpEvent*>(e);
            QPoint relpos = help->pos();
            bool eventAccepted = help->isAccepted();
            while (w) {
                QHelpEvent he(help->type(), relpos, help->globalPos());
                he.spont = e->spontaneous();
                res = d->notify_helper(w, w == receiver ? help : &he);
                e->spont = false;
                eventAccepted = (w == receiver ? help : &he)->isAccepted();
                if ((res && eventAccepted) || w->isWindow())
                    break;

                relpos += w->pos();
                w = w->parentWidget();
            }
            help->setAccepted(eventAccepted);
        }
        break;
#endif
#if QT_CONFIG(statustip) || QT_CONFIG(whatsthis)
    case QEvent::StatusTip:
    case QEvent::WhatsThisClicked:
        {
            QWidget *w = static_cast<QWidget *>(receiver);
            while (w) {
                res = d->notify_helper(w, e);
                if ((res && e->isAccepted()) || w->isWindow())
                    break;
                w = w->parentWidget();
            }
        }
        break;
#endif

#if QT_CONFIG(draganddrop)
    case QEvent::DragEnter: {
            QWidget* w = static_cast<QWidget *>(receiver);
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(e);
#if QT_CONFIG(graphicsview)
            // QGraphicsProxyWidget handles its own propagation,
            // and we must not change QDragManagers currentTarget.
            const auto &extra = w->window()->d_func()->extra;
            if (extra && extra->proxyWidget) {
                res = d->notify_helper(w, dragEvent);
                break;
            }
#endif
            while (w) {
                if (w->isEnabled() && w->acceptDrops()) {
                    res = d->notify_helper(w, dragEvent);
                    if (res && dragEvent->isAccepted()) {
                        QDragManager::self()->setCurrentTarget(w);
                        break;
                    }
                }
                if (w->isWindow())
                    break;
                dragEvent->p = w->mapToParent(dragEvent->p.toPoint());
                w = w->parentWidget();
            }
        }
        break;
    case QEvent::DragMove:
    case QEvent::Drop:
    case QEvent::DragLeave: {
            QWidget* w = static_cast<QWidget *>(receiver);
#if QT_CONFIG(graphicsview)
            // QGraphicsProxyWidget handles its own propagation,
            // and we must not change QDragManagers currentTarget.
            const auto &extra = w->window()->d_func()->extra;
            bool isProxyWidget = extra && extra->proxyWidget;
            if (!isProxyWidget)
#endif
                w = qobject_cast<QWidget *>(QDragManager::self()->currentTarget());

            if (!w) {
                    break;
            }
            if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop) {
                QDropEvent *dragEvent = static_cast<QDropEvent *>(e);
                QWidget *origReciver = static_cast<QWidget *>(receiver);
                while (origReciver && w != origReciver) {
                    dragEvent->p = origReciver->mapToParent(dragEvent->p.toPoint());
                    origReciver = origReciver->parentWidget();
                }
            }
            res = d->notify_helper(w, e);
            if (e->type() != QEvent::DragMove
#if QT_CONFIG(graphicsview)
                && !isProxyWidget
#endif
                )
                QDragManager::self()->setCurrentTarget(nullptr, e->type() == QEvent::Drop);
        }
        break;
#endif
    case QEvent::TouchBegin:
    // Note: TouchUpdate and TouchEnd events are never propagated
    {
        QWidget *widget = static_cast<QWidget *>(receiver);
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(e);
        bool eventAccepted = touchEvent->isAccepted();
        bool acceptTouchEvents = widget->testAttribute(Qt::WA_AcceptTouchEvents);

        if (acceptTouchEvents && e->spontaneous()) {
            const QPoint localPos = touchEvent->touchPoints()[0].pos().toPoint();
            QApplicationPrivate::giveFocusAccordingToFocusPolicy(widget, e, localPos);
        }

#ifndef QT_NO_GESTURES
        QPointer<QWidget> gesturePendingWidget;
#endif

        while (widget) {
            // first, try to deliver the touch event
            acceptTouchEvents = widget->testAttribute(Qt::WA_AcceptTouchEvents);
            touchEvent->setTarget(widget);
            touchEvent->setAccepted(acceptTouchEvents);
            QPointer<QWidget> p = widget;
            res = acceptTouchEvents && d->notify_helper(widget, touchEvent);
            eventAccepted = touchEvent->isAccepted();
            if (p.isNull()) {
                // widget was deleted
                widget = nullptr;
            } else {
                widget->setAttribute(Qt::WA_WState_AcceptedTouchBeginEvent, res && eventAccepted);
            }
            touchEvent->spont = false;
            if (res && eventAccepted) {
                // the first widget to accept the TouchBegin gets an implicit grab.
                d->activateImplicitTouchGrab(widget, touchEvent);
                break;
            }
#ifndef QT_NO_GESTURES
            if (gesturePendingWidget.isNull() && widget && QGestureManager::gesturePending(widget))
                gesturePendingWidget = widget;
#endif
            if (p.isNull() || widget->isWindow() || widget->testAttribute(Qt::WA_NoMousePropagation))
                break;

            QPoint offset = widget->pos();
            widget = widget->parentWidget();
            touchEvent->setTarget(widget);
            for (int i = 0; i < touchEvent->_touchPoints.size(); ++i) {
                QTouchEvent::TouchPoint &pt = touchEvent->_touchPoints[i];
                pt.d->pos = pt.pos() + offset;
                pt.d->startPos = pt.startPos() + offset;
                pt.d->lastPos = pt.lastPos() + offset;
            }
        }

#ifndef QT_NO_GESTURES
        if (!eventAccepted && !gesturePendingWidget.isNull()) {
            // the first widget subscribed to a gesture gets an implicit grab
            d->activateImplicitTouchGrab(gesturePendingWidget, touchEvent);
        }
#endif

        touchEvent->setAccepted(eventAccepted);
        break;
    }
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QWidget *widget = static_cast<QWidget *>(receiver);
        // We may get here if the widget is subscribed to a gesture,
        // but has not accepted TouchBegin. Propagate touch events
        // only if TouchBegin has been accepted.
        if (widget->testAttribute(Qt::WA_WState_AcceptedTouchBeginEvent))
            res = d->notify_helper(widget, e);
        break;
    }
    case QEvent::RequestSoftwareInputPanel:
        inputMethod()->show();
        break;
    case QEvent::CloseSoftwareInputPanel:
        inputMethod()->hide();
        break;

#ifndef QT_NO_GESTURES
    case QEvent::NativeGesture:
    {
        // only propagate the first gesture event (after the GID_BEGIN)
        QWidget *w = static_cast<QWidget *>(receiver);
        while (w) {
            e->ignore();
            res = d->notify_helper(w, e);
            if ((res && e->isAccepted()) || w->isWindow())
                break;
            w = w->parentWidget();
        }
        break;
    }
    case QEvent::Gesture:
    case QEvent::GestureOverride:
    {
        if (receiver->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(receiver);
            QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(e);
            QList<QGesture *> allGestures = gestureEvent->gestures();

            bool eventAccepted = gestureEvent->isAccepted();
            bool wasAccepted = eventAccepted;
            while (w) {
                // send only gestures the widget expects
                QList<QGesture *> gestures;
                QWidgetPrivate *wd = w->d_func();
                for (int i = 0; i < allGestures.size();) {
                    QGesture *g = allGestures.at(i);
                    Qt::GestureType type = g->gestureType();
                    QMap<Qt::GestureType, Qt::GestureFlags>::iterator contextit =
                            wd->gestureContext.find(type);
                    bool deliver = contextit != wd->gestureContext.end() &&
                        (g->state() == Qt::GestureStarted || w == receiver ||
                         (contextit.value() & Qt::ReceivePartialGestures));
                    if (deliver) {
                        allGestures.removeAt(i);
                        gestures.append(g);
                    } else {
                        ++i;
                    }
                }
                if (!gestures.isEmpty()) { // we have gestures for this w
                    QGestureEvent ge(gestures);
                    ge.t = gestureEvent->t;
                    ge.spont = gestureEvent->spont;
                    ge.m_accept = wasAccepted;
                    ge.m_accepted = gestureEvent->m_accepted;
                    res = d->notify_helper(w, &ge);
                    gestureEvent->spont = false;
                    eventAccepted = ge.isAccepted();
                    for (int i = 0; i < gestures.size(); ++i) {
                        QGesture *g = gestures.at(i);
                        // Ignore res [event return value] because handling of multiple gestures
                        // packed into a single QEvent depends on not consuming the event
                        if (eventAccepted || ge.isAccepted(g)) {
                            // if the gesture was accepted, mark the target widget for it
                            gestureEvent->m_targetWidgets[g->gestureType()] = w;
                            gestureEvent->setAccepted(g, true);
                        } else {
                            // if the gesture was explicitly ignored by the application,
                            // put it back so a parent can get it
                            allGestures.append(g);
                        }
                    }
                }
                if (allGestures.isEmpty()) // everything delivered
                    break;
                if (w->isWindow())
                    break;
                w = w->parentWidget();
            }
            for (QGesture *g : qAsConst(allGestures))
                gestureEvent->setAccepted(g, false);
            gestureEvent->m_accept = false; // to make sure we check individual gestures
        } else {
            res = d->notify_helper(receiver, e);
        }
        break;
    }
#endif // QT_NO_GESTURES
#ifdef Q_OS_MAC
    // Enable touch events on enter, disable on leave.
    typedef void (*RegisterTouchWindowFn)(QWindow *,  bool);
    case QEvent::Enter:
        if (receiver->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(receiver);
            if (w->testAttribute(Qt::WA_AcceptTouchEvents)) {
                RegisterTouchWindowFn registerTouchWindow = reinterpret_cast<RegisterTouchWindowFn>
                        (platformNativeInterface()->nativeResourceFunctionForIntegration("registertouchwindow"));
                if (registerTouchWindow)
                    registerTouchWindow(w->window()->windowHandle(), true);
            }
        }
        res = d->notify_helper(receiver, e);
    break;
    case QEvent::Leave:
        if (receiver->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(receiver);
            if (w->testAttribute(Qt::WA_AcceptTouchEvents)) {
                RegisterTouchWindowFn registerTouchWindow = reinterpret_cast<RegisterTouchWindowFn>
                        (platformNativeInterface()->nativeResourceFunctionForIntegration("registertouchwindow"));
                if (registerTouchWindow)
                    registerTouchWindow(w->window()->windowHandle(), false);
            }
        }
        res = d->notify_helper(receiver, e);
    break;
#endif
    default:
        res = d->notify_helper(receiver, e);
        break;
    }

    return res;
}

bool QApplicationPrivate::notify_helper(QObject *receiver, QEvent * e)
{
    // These tracepoints (and the whole function, actually) are very similar
    // to the ones in QCoreApplicationPrivate::notify_helper; the reason for their
    // duplication is because tracepoint symbols are not exported by QtCore.
    // If you adjust the tracepoints here, consider adjusting QCoreApplicationPrivate too.
    Q_TRACE(QApplication_notify_entry, receiver, e, e->type());
    bool consumed = false;
    bool filtered = false;
    Q_TRACE_EXIT(QApplication_notify_exit, consumed, filtered);

    // send to all application event filters
    if (threadRequiresCoreApplication()
        && receiver->d_func()->threadData.loadRelaxed()->thread.loadAcquire() == mainThread()
        && sendThroughApplicationEventFilters(receiver, e)) {
        filtered = true;
        return filtered;
    }

    if (receiver->isWidgetType()) {
        QWidget *widget = static_cast<QWidget *>(receiver);

#if !defined(QT_NO_CURSOR)
        // toggle HasMouse widget state on enter and leave
        if ((e->type() == QEvent::Enter || e->type() == QEvent::DragEnter) &&
            (!QApplication::activePopupWidget() || QApplication::activePopupWidget() == widget->window()))
            widget->setAttribute(Qt::WA_UnderMouse, true);
        else if (e->type() == QEvent::Leave || e->type() == QEvent::DragLeave)
            widget->setAttribute(Qt::WA_UnderMouse, false);
#endif

        if (QLayout *layout=widget->d_func()->layout) {
            layout->widgetEvent(e);
        }
    }

    // send to all receiver event filters
    if (sendThroughObjectEventFilters(receiver, e)) {
        filtered = true;
        return filtered;
    }

    // deliver the event
    consumed = receiver->event(e);

    QCoreApplicationPrivate::setEventSpontaneous(e, false);
    return consumed;
}

bool QApplicationPrivate::inPopupMode()
{
    return QApplicationPrivate::popupWidgets != nullptr;
}

static void ungrabKeyboardForPopup(QWidget *popup)
{
    if (QWidget::keyboardGrabber())
        qt_widget_private(QWidget::keyboardGrabber())->stealKeyboardGrab(true);
    else
        qt_widget_private(popup)->stealKeyboardGrab(false);
}

static void ungrabMouseForPopup(QWidget *popup)
{
    if (QWidget::mouseGrabber())
        qt_widget_private(QWidget::mouseGrabber())->stealMouseGrab(true);
    else
        qt_widget_private(popup)->stealMouseGrab(false);
}

static bool popupGrabOk;

static void grabForPopup(QWidget *popup)
{
    Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
    popupGrabOk = qt_widget_private(popup)->stealKeyboardGrab(true);
    if (popupGrabOk) {
        popupGrabOk = qt_widget_private(popup)->stealMouseGrab(true);
        if (!popupGrabOk) {
            // transfer grab back to the keyboard grabber if any
            ungrabKeyboardForPopup(popup);
        }
    }
}

extern QWidget *qt_popup_down;
extern bool qt_replay_popup_mouse_event;
extern bool qt_popup_down_closed;

void QApplicationPrivate::closePopup(QWidget *popup)
{
    if (!popupWidgets)
        return;
    popupWidgets->removeAll(popup);

     if (popup == qt_popup_down) {
         qt_button_down = nullptr;
         qt_popup_down_closed = true;
         qt_popup_down = nullptr;
     }

    if (QApplicationPrivate::popupWidgets->count() == 0) { // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = nullptr;
        qt_popup_down_closed = false;

        if (popupGrabOk) {
            popupGrabOk = false;

            if (popup->geometry().contains(QPoint(QGuiApplicationPrivate::mousePressX,
                                                  QGuiApplicationPrivate::mousePressY))
                || popup->testAttribute(Qt::WA_NoMouseReplay)) {
                // mouse release event or inside
                qt_replay_popup_mouse_event = false;
            } else { // mouse press event
                qt_replay_popup_mouse_event = true;
            }

            // transfer grab back to mouse grabber if any, otherwise release the grab
            ungrabMouseForPopup(popup);

            // transfer grab back to keyboard grabber if any, otherwise release the grab
            ungrabKeyboardForPopup(popup);
        }

        if (active_window) {
            if (QWidget *fw = active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    QCoreApplication::sendEvent(fw, &e);
                }
            }
        }

    } else {
        // A popup was closed, so the previous popup gets the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->constLast();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        // can become nullptr due to setFocus() above
        if (QApplicationPrivate::popupWidgets &&
            QApplicationPrivate::popupWidgets->count() == 1) // grab mouse/keyboard
            grabForPopup(aw);
    }

}

int openPopupCount = 0;

void QApplicationPrivate::openPopup(QWidget *popup)
{
    openPopupCount++;
    if (!popupWidgets) // create list
        popupWidgets = new QWidgetList;
    popupWidgets->append(popup); // add to end of list

    if (QApplicationPrivate::popupWidgets->count() == 1) // grab mouse/keyboard
        grabForPopup(popup);

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            QCoreApplication::sendEvent(fw, &e);
        }
    }
}

#ifdef QT_KEYPAD_NAVIGATION
/*!
    Sets the kind of focus navigation Qt should use to \a mode.

    This feature is available in Qt for Embedded Linux only.

    \since 4.6
*/
void QApplication::setNavigationMode(Qt::NavigationMode mode)
{
    QApplicationPrivate::navigationMode = mode;
}

/*!
    Returns what kind of focus navigation Qt is using.

    This feature is available in Qt for Embedded Linux only.

    \since 4.6
*/
Qt::NavigationMode QApplication::navigationMode()
{
    return QApplicationPrivate::navigationMode;
}

# if QT_DEPRECATED_SINCE(5, 13)
/*!
    Sets whether Qt should use focus navigation suitable for use with a
    minimal keypad.

    This feature is available in Qt for Embedded Linux, and Windows CE only.

    \note On Windows CE this feature is disabled by default for touch device
          mkspecs. To enable keypad navigation, build Qt with
          QT_KEYPAD_NAVIGATION defined.

    \deprecated

    \sa setNavigationMode()
*/
void QApplication::setKeypadNavigationEnabled(bool enable)
{
    if (enable) {
        QApplication::setNavigationMode(Qt::NavigationModeKeypadTabOrder);
    } else {
        QApplication::setNavigationMode(Qt::NavigationModeNone);
    }
}

/*!
    Returns \c true if Qt is set to use keypad navigation; otherwise returns
    false.  The default value is false.

    This feature is available in Qt for Embedded Linux, and Windows CE only.

    \note On Windows CE this feature is disabled by default for touch device
          mkspecs. To enable keypad navigation, build Qt with
          QT_KEYPAD_NAVIGATION defined.

    \deprecated

    \sa navigationMode()
*/
bool QApplication::keypadNavigationEnabled()
{
    return QApplicationPrivate::navigationMode == Qt::NavigationModeKeypadTabOrder ||
        QApplicationPrivate::navigationMode == Qt::NavigationModeKeypadDirectional;
}
# endif
#endif

/*!
    \fn void QApplication::alert(QWidget *widget, int msec)
    \since 4.3

    Causes an alert to be shown for \a widget if the window is not the active
    window. The alert is shown for \a msec miliseconds. If \a msec is zero (the
    default), then the alert is shown indefinitely until the window becomes
    active again.

    Currently this function does nothing on Qt for Embedded Linux.

    On \macos, this works more at the application level and will cause the
    application icon to bounce in the dock.

    On Windows, this causes the window's taskbar entry to flash for a time. If
    \a msec is zero, the flashing will stop and the taskbar entry will turn a
    different color (currently orange).

    On X11, this will cause the window to be marked as "demands attention", the
    window must not be hidden (i.e. not have hide() called on it, but be
    visible in some sort of way) in order for this to work.
*/
void QApplication::alert(QWidget *widget, int duration)
{
    if (widget) {
       if (widget->window()->isActiveWindow() && !(widget->window()->windowState() & Qt::WindowMinimized))
            return;
        if (QWindow *window= QApplicationPrivate::windowForWidget(widget))
            window->alert(duration);
    } else {
        const auto topLevels = topLevelWidgets();
        for (QWidget *topLevel : topLevels)
            QApplication::alert(topLevel, duration);
    }
}

/*!
    \property QApplication::cursorFlashTime
    \brief the text cursor's flash (blink) time in milliseconds

    The flash time is the time required to display, invert and restore the
    caret display. Usually the text cursor is displayed for half the cursor
    flash time, then hidden for the same amount of time, but this may vary.

    The default value on X11 is 1000 milliseconds. On Windows, the
    \uicontrol{Control Panel} value is used and setting this property sets the cursor
    flash time for all applications.

    We recommend that widgets do not cache this value as it may change at any
    time if the user changes the global desktop settings.

    \note This property may hold a negative value, for instance if cursor
    blinking is disabled.
*/
void QApplication::setCursorFlashTime(int msecs)
{
    QGuiApplication::styleHints()->setCursorFlashTime(msecs);
}

int QApplication::cursorFlashTime()
{
    return QGuiApplication::styleHints()->cursorFlashTime();
}

/*!
    \property QApplication::doubleClickInterval
    \brief the time limit in milliseconds that distinguishes a double click
    from two consecutive mouse clicks

    The default value on X11 is 400 milliseconds. On Windows and Mac OS, the
    operating system's value is used.
*/
void QApplication::setDoubleClickInterval(int ms)
{
    QGuiApplication::styleHints()->setMouseDoubleClickInterval(ms);
}

int QApplication::doubleClickInterval()
{
    return QGuiApplication::styleHints()->mouseDoubleClickInterval();
}

/*!
    \property QApplication::keyboardInputInterval
    \brief the time limit in milliseconds that distinguishes a key press
    from two consecutive key presses
    \since 4.2

    The default value on X11 is 400 milliseconds. On Windows and Mac OS, the
    operating system's value is used.
*/
void QApplication::setKeyboardInputInterval(int ms)
{
    QGuiApplication::styleHints()->setKeyboardInputInterval(ms);
}

int QApplication::keyboardInputInterval()
{
    return QGuiApplication::styleHints()->keyboardInputInterval();
}

/*!
    \property QApplication::wheelScrollLines
    \brief the number of lines to scroll a widget, when the
    mouse wheel is rotated.

    If the value exceeds the widget's number of visible lines, the widget
    should interpret the scroll operation as a single \e{page up} or
    \e{page down}. If the widget is an \l{QAbstractItemView}{item view class},
    then the result of scrolling one \e line depends on the setting of the
    widget's \l{QAbstractItemView::verticalScrollMode()}{scroll mode}. Scroll
    one \e line can mean \l{QAbstractItemView::ScrollPerItem}{scroll one item}
    or \l{QAbstractItemView::ScrollPerPixel}{scroll one pixel}.

    By default, this property has a value of 3.

    \sa QStyleHints::wheelScrollLines()
*/
#if QT_CONFIG(wheelevent)
int QApplication::wheelScrollLines()
{
    return styleHints()->wheelScrollLines();
}

void QApplication::setWheelScrollLines(int lines)
{
    styleHints()->setWheelScrollLines(lines);
}
#endif

static inline int uiEffectToFlag(Qt::UIEffect effect)
{
    switch (effect) {
    case Qt::UI_General:
        return QPlatformTheme::GeneralUiEffect;
    case Qt::UI_AnimateMenu:
        return QPlatformTheme::AnimateMenuUiEffect;
    case Qt::UI_FadeMenu:
        return QPlatformTheme::FadeMenuUiEffect;
    case Qt::UI_AnimateCombo:
        return QPlatformTheme::AnimateComboUiEffect;
    case Qt::UI_AnimateTooltip:
        return QPlatformTheme::AnimateTooltipUiEffect;
    case Qt::UI_FadeTooltip:
        return QPlatformTheme::FadeTooltipUiEffect;
    case Qt::UI_AnimateToolBox:
        return QPlatformTheme::AnimateToolBoxUiEffect;
    }
    return 0;
}

/*!
    \fn void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)

    Enables the UI effect \a effect if \a enable is true, otherwise the effect
    will not be used.

    \note All effects are disabled on screens running at less than 16-bit color
    depth.

    \sa isEffectEnabled(), Qt::UIEffect, setDesktopSettingsAware()
*/
void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    int effectFlags = uiEffectToFlag(effect);
    if (enable) {
        if (effectFlags & QPlatformTheme::FadeMenuUiEffect)
            effectFlags |= QPlatformTheme::AnimateMenuUiEffect;
        if (effectFlags & QPlatformTheme::FadeTooltipUiEffect)
            effectFlags |= QPlatformTheme::AnimateTooltipUiEffect;
        QApplicationPrivate::enabledAnimations |= effectFlags;
    } else {
        QApplicationPrivate::enabledAnimations &= ~effectFlags;
    }
}

/*!
    \fn bool QApplication::isEffectEnabled(Qt::UIEffect effect)

    Returns \c true if \a effect is enabled; otherwise returns \c false.

    By default, Qt will try to use the desktop settings. To prevent this, call
    setDesktopSettingsAware(false).

    \note All effects are disabled on screens running at less than 16-bit color
    depth.

    \sa setEffectEnabled(), Qt::UIEffect
*/
bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    CHECK_QAPP_INSTANCE(false)
    return QColormap::instance().depth() >= 16
           && (QApplicationPrivate::enabledAnimations & QPlatformTheme::GeneralUiEffect)
           && (QApplicationPrivate::enabledAnimations & uiEffectToFlag(effect));
}

/*!
    \fn void QApplication::beep()

    Sounds the bell, using the default volume and sound. The function is \e not
    available in Qt for Embedded Linux.
*/
void QApplication::beep()
{
    QGuiApplicationPrivate::platformIntegration()->beep();
}

/*!
    \macro qApp
    \relates QApplication

    A global pointer referring to the unique application object. It is
    equivalent to QCoreApplication::instance(), but cast as a QApplication pointer,
    so only valid when the unique application object is a QApplication.

    \sa QCoreApplication::instance(), qGuiApp
*/

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event)
{
    return QGuiApplication::sendSpontaneousEvent(receiver, event);
}

void QApplicationPrivate::giveFocusAccordingToFocusPolicy(QWidget *widget, QEvent *event, QPoint localPos)
{
    const bool setFocusOnRelease = QGuiApplication::styleHints()->setFocusOnTouchRelease();
    Qt::FocusPolicy focusPolicy = Qt::ClickFocus;
    static QPointer<QWidget> focusedWidgetOnTouchBegin = nullptr;

    switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::TouchBegin:
            focusedWidgetOnTouchBegin = QApplication::focusWidget();
            if (setFocusOnRelease)
                return;
            break;
        case QEvent::MouseButtonRelease:
        case QEvent::TouchEnd:
            if (!setFocusOnRelease)
                return;
            if (focusedWidgetOnTouchBegin != QApplication::focusWidget()) {
                // Focus widget was changed while delivering press/move events.
                // To not interfere with application logic, we leave focus as-is
                return;
            }
            break;
        case QEvent::Wheel:
            focusPolicy = Qt::WheelFocus;
            break;
        default:
            return;
    }

    QWidget *focusWidget = widget;
    while (focusWidget) {
        if (focusWidget->isEnabled()
            && focusWidget->rect().contains(localPos)
            && QApplicationPrivate::shouldSetFocus(focusWidget, focusPolicy)) {
            focusWidget->setFocus(Qt::MouseFocusReason);
            break;
        }
        if (focusWidget->isWindow())
            break;

        // find out whether this widget (or its proxy) already has focus
        QWidget *f = focusWidget;
        if (focusWidget->d_func()->extra && focusWidget->d_func()->extra->focus_proxy)
            f = focusWidget->d_func()->extra->focus_proxy;
        // if it has, stop here.
        // otherwise a click on the focused widget would remove its focus if ClickFocus isn't set
        if (f->hasFocus())
            break;

        localPos += focusWidget->pos();
        focusWidget = focusWidget->parentWidget();
    }
}

bool QApplicationPrivate::shouldSetFocus(QWidget *w, Qt::FocusPolicy policy)
{
    QWidget *f = w;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if ((w->focusPolicy() & policy) != policy)
        return false;
    if (w != f && (f->focusPolicy() & policy) != policy)
        return false;
    return true;
}

bool QApplicationPrivate::updateTouchPointsForWidget(QWidget *widget, QTouchEvent *touchEvent)
{
    bool containsPress = false;
    for (int i = 0; i < touchEvent->touchPoints().count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = touchEvent->_touchPoints[i];

        // preserve the sub-pixel resolution
        const QPointF screenPos = touchPoint.screenPos();
        const QPointF delta = screenPos - screenPos.toPoint();

        touchPoint.d->pos = widget->mapFromGlobal(screenPos.toPoint()) + delta;
        touchPoint.d->startPos = widget->mapFromGlobal(touchPoint.startScreenPos().toPoint()) + delta;
        touchPoint.d->lastPos = widget->mapFromGlobal(touchPoint.lastScreenPos().toPoint()) + delta;

        if (touchPoint.state() == Qt::TouchPointPressed)
            containsPress = true;
    }
    return containsPress;
}

void QApplicationPrivate::initializeMultitouch()
{
    initializeMultitouch_sys();
}

void QApplicationPrivate::initializeMultitouch_sys()
{
}

void QApplicationPrivate::cleanupMultitouch()
{
    cleanupMultitouch_sys();
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
}

QWidget *QApplicationPrivate::findClosestTouchPointTarget(QTouchDevice *device, const QTouchEvent::TouchPoint &touchPoint)
{
    const QPointF screenPos = touchPoint.screenPos();
    int closestTouchPointId = -1;
    QObject *closestTarget = nullptr;
    qreal closestDistance = qreal(0.);
    QHash<ActiveTouchPointsKey, ActiveTouchPointsValue>::const_iterator it = activeTouchPoints.constBegin(),
            ite = activeTouchPoints.constEnd();
    while (it != ite) {
        if (it.key().device == device && it.key().touchPointId != touchPoint.id()) {
            const QTouchEvent::TouchPoint &touchPoint = it->touchPoint;
            qreal dx = screenPos.x() - touchPoint.screenPos().x();
            qreal dy = screenPos.y() - touchPoint.screenPos().y();
            qreal distance = dx * dx + dy * dy;
            if (closestTouchPointId == -1 || distance < closestDistance) {
                closestTouchPointId = touchPoint.id();
                closestDistance = distance;
                closestTarget = it.value().target.data();
            }
        }
        ++it;
    }
    return static_cast<QWidget *>(closestTarget);
}

void QApplicationPrivate::activateImplicitTouchGrab(QWidget *widget, QTouchEvent *touchEvent)
{
    if (touchEvent->type() != QEvent::TouchBegin)
        return;

    for (int i = 0, tc = touchEvent->touchPoints().count(); i < tc; ++i) {
        const QTouchEvent::TouchPoint &touchPoint = touchEvent->touchPoints().at(i);
        activeTouchPoints[QGuiApplicationPrivate::ActiveTouchPointsKey(touchEvent->device(), touchPoint.id())].target = widget;
    }
}

bool QApplicationPrivate::translateRawTouchEvent(QWidget *window,
                                                 QTouchDevice *device,
                                                 const QList<QTouchEvent::TouchPoint> &touchPoints,
                                                 ulong timestamp)
{
    QApplicationPrivate *d = self;
    typedef QPair<Qt::TouchPointStates, QList<QTouchEvent::TouchPoint> > StatesAndTouchPoints;
    QHash<QWidget *, StatesAndTouchPoints> widgetsNeedingEvents;

    for (int i = 0; i < touchPoints.count(); ++i) {
        QTouchEvent::TouchPoint touchPoint = touchPoints.at(i);
        // explicitly detach from the original touch point that we got, so even
        // if the touchpoint structs are reused, we will make a copy that we'll
        // deliver to the user (which might want to store the struct for later use).
        touchPoint.d = touchPoint.d->detach();

        // update state
        QPointer<QObject> target;
        ActiveTouchPointsKey touchInfoKey(device, touchPoint.id());
        ActiveTouchPointsValue &touchInfo = d->activeTouchPoints[touchInfoKey];
        if (touchPoint.state() == Qt::TouchPointPressed) {
            if (device->type() == QTouchDevice::TouchPad) {
                // on touch-pads, send all touch points to the same widget
                target = d->activeTouchPoints.isEmpty()
                        ? QPointer<QObject>()
                        : d->activeTouchPoints.constBegin().value().target;
            }

            if (!target) {
                // determine which widget this event will go to
                if (!window)
                    window = QApplication::topLevelAt(touchPoint.screenPos().toPoint());
                if (!window)
                    continue;
                target = window->childAt(window->mapFromGlobal(touchPoint.screenPos().toPoint()));
                if (!target)
                    target = window;
            }

            if (device->type() == QTouchDevice::TouchScreen) {
                QWidget *closestWidget = d->findClosestTouchPointTarget(device, touchPoint);
                QWidget *widget = static_cast<QWidget *>(target.data());
                if (closestWidget
                        && (widget->isAncestorOf(closestWidget) || closestWidget->isAncestorOf(widget))) {
                    target = closestWidget;
                }
            }

            touchInfo.target = target;
        } else {
            target = touchInfo.target;
            if (!target)
                continue;
        }
        Q_ASSERT(target.data() != nullptr);

        QWidget *targetWidget = static_cast<QWidget *>(target.data());

#ifdef Q_OS_MACOS
        // Single-touch events are normally not sent unless WA_TouchPadAcceptSingleTouchEvents is set.
        // In Qt 4 this check was in OS X-only code. That behavior is preserved here by the #ifdef.
        if (touchPoints.count() == 1
            && device->type() == QTouchDevice::TouchPad
            && !targetWidget->testAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents))
            continue;
#endif

        StatesAndTouchPoints &maskAndPoints = widgetsNeedingEvents[targetWidget];
        maskAndPoints.first |= touchPoint.state();
        maskAndPoints.second.append(touchPoint);
    }

    if (widgetsNeedingEvents.isEmpty())
        return false;

    bool accepted = false;
    QHash<QWidget *, StatesAndTouchPoints>::ConstIterator it = widgetsNeedingEvents.constBegin();
    const QHash<QWidget *, StatesAndTouchPoints>::ConstIterator end = widgetsNeedingEvents.constEnd();
    for (; it != end; ++it) {
        const QPointer<QWidget> widget = it.key();
        if (!QApplicationPrivate::tryModalHelper(widget, nullptr))
            continue;

        QEvent::Type eventType;
        switch (it.value().first) {
        case Qt::TouchPointPressed:
            eventType = QEvent::TouchBegin;
            break;
        case Qt::TouchPointReleased:
            eventType = QEvent::TouchEnd;
            break;
        case Qt::TouchPointStationary:
            // don't send the event if nothing changed
            continue;
        default:
            eventType = QEvent::TouchUpdate;
            break;
        }

        QTouchEvent touchEvent(eventType,
                               device,
                               QGuiApplication::keyboardModifiers(),
                               it.value().first,
                               it.value().second);
        bool containsPress = updateTouchPointsForWidget(widget, &touchEvent);
        touchEvent.setTimestamp(timestamp);
        touchEvent.setWindow(window->windowHandle());
        touchEvent.setTarget(widget);

        if (containsPress)
            widget->setAttribute(Qt::WA_WState_AcceptedTouchBeginEvent);

        switch (touchEvent.type()) {
        case QEvent::TouchBegin:
        {
            // if the TouchBegin handler recurses, we assume that means the event
            // has been implicitly accepted and continue to send touch events
            if (QApplication::sendSpontaneousEvent(widget, &touchEvent) && touchEvent.isAccepted()) {
                accepted = true;
                if (!widget.isNull())
                    widget->setAttribute(Qt::WA_WState_AcceptedTouchBeginEvent);
            }
            break;
        }
        default:
            if (widget->testAttribute(Qt::WA_WState_AcceptedTouchBeginEvent)
#ifndef QT_NO_GESTURES
                || QGestureManager::gesturePending(widget)
#endif
                ) {
                if (QApplication::sendSpontaneousEvent(widget, &touchEvent) && touchEvent.isAccepted())
                    accepted = true;
                // widget can be deleted on TouchEnd
                if (touchEvent.type() == QEvent::TouchEnd && !widget.isNull())
                    widget->setAttribute(Qt::WA_WState_AcceptedTouchBeginEvent, false);
            }
            break;
        }
    }
    return accepted;
}

void QApplicationPrivate::translateTouchCancel(QTouchDevice *device, ulong timestamp)
{
    QTouchEvent touchEvent(QEvent::TouchCancel, device, QGuiApplication::keyboardModifiers());
    touchEvent.setTimestamp(timestamp);
    QHash<ActiveTouchPointsKey, ActiveTouchPointsValue>::const_iterator it
            = self->activeTouchPoints.constBegin(), ite = self->activeTouchPoints.constEnd();
    QSet<QWidget *> widgetsNeedingCancel;
    while (it != ite) {
        QWidget *widget = static_cast<QWidget *>(it->target.data());
        if (widget)
            widgetsNeedingCancel.insert(widget);
        ++it;
    }
    for (QSet<QWidget *>::const_iterator widIt = widgetsNeedingCancel.constBegin(),
         widItEnd = widgetsNeedingCancel.constEnd(); widIt != widItEnd; ++widIt) {
        QWidget *widget = *widIt;
        touchEvent.setWindow(widget->windowHandle());
        touchEvent.setTarget(widget);
        QApplication::sendSpontaneousEvent(widget, &touchEvent);
    }
}

void QApplicationPrivate::notifyThemeChanged()
{
    QGuiApplicationPrivate::notifyThemeChanged();

    qt_init_tooltip_palette();
}

#if QT_CONFIG(draganddrop)
void QApplicationPrivate::notifyDragStarted(const QDrag *drag)
{
    QGuiApplicationPrivate::notifyDragStarted(drag);
    // QTBUG-26145
    // Prevent pickMouseReceiver() from using the widget where the drag was started after a drag operation...
    // QTBUG-56713
    // ...only if qt_button_down is not a QQuickWidget
    if (qt_button_down && !qt_button_down->inherits("QQuickWidget"))
        qt_button_down = nullptr;
}
#endif // QT_CONFIG(draganddrop)

#ifndef QT_NO_GESTURES
QGestureManager* QGestureManager::instance(InstanceCreation ic)
{
    QApplicationPrivate *qAppPriv = QApplicationPrivate::instance();
    if (!qAppPriv)
        return nullptr;
    if (!qAppPriv->gestureManager && ic == ForceCreation)
        qAppPriv->gestureManager = new QGestureManager(qApp);
    return qAppPriv->gestureManager;
}
#endif // QT_NO_GESTURES

QPixmap QApplicationPrivate::applyQIconStyleHelper(QIcon::Mode mode, const QPixmap& base) const
{
    QStyleOption opt(0);
    opt.palette = QGuiApplication::palette();
    return QApplication::style()->generatedIconPixmap(mode, base, &opt);
}

QT_END_NAMESPACE

#include "moc_qapplication.cpp"
