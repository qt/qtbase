/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qguiapplication.h"

#include "private/qguiapplication_p.h"
#include "private/qplatformintegrationfactory_qpa_p.h"
#include "private/qevent_p.h"

#if !defined(QT_NO_GLIB)
#include "qeventdispatcher_glib_qpa_p.h"
#endif
#include "qeventdispatcher_qpa_p.h"

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>
#include <QtCore/qmutex.h>
#include <QtDebug>

#include <QtGui/QPlatformIntegration>
#include <QtGui/QGenericPluginFactory>

#include <QWindowSystemInterface>
#include "private/qwindowsysteminterface_qpa_p.h"
#include "private/qwindow_p.h"
#include "private/qkeymapper_p.h"

#include <QtGui/QPixmap>

#ifndef QT_NO_CLIPBOARD
#include <QtGui/QClipboard>
#endif

QT_BEGIN_NAMESPACE

Qt::MouseButtons QGuiApplicationPrivate::mouse_buttons = Qt::NoButton;
Qt::KeyboardModifiers QGuiApplicationPrivate::modifier_buttons = Qt::NoModifier;

int QGuiApplicationPrivate::keyboard_input_time = 0;
int QGuiApplicationPrivate::mouse_double_click_time = 0;

QPlatformIntegration *QGuiApplicationPrivate::platform_integration = 0;

bool QGuiApplicationPrivate::app_do_modal = false;

int qt_last_x = 0;
int qt_last_y = 0;

Qt::MouseButtons QGuiApplicationPrivate::buttons = Qt::NoButton;
ulong QGuiApplicationPrivate::mousePressTime = 0;
Qt::MouseButton QGuiApplicationPrivate::mousePressButton = Qt::NoButton;
int QGuiApplicationPrivate::mousePressX = 0;
int QGuiApplicationPrivate::mousePressY = 0;
int QGuiApplicationPrivate::mouse_double_click_distance = 5;

static Qt::LayoutDirection layout_direction = Qt::LeftToRight;
static bool force_reverse = false;

QGuiApplicationPrivate *QGuiApplicationPrivate::self = 0;

#ifndef QT_NO_CLIPBOARD
QClipboard *QGuiApplicationPrivate::qt_clipboard = 0;
#endif

QWindowList QGuiApplicationPrivate::window_list;

Q_GLOBAL_STATIC(QMutex, applicationFontMutex)
QFont *QGuiApplicationPrivate::app_font = 0;

static bool qt_detectRTLLanguage()
{
    return force_reverse ^
        (QCoreApplication::tr("QT_LAYOUT_DIRECTION",
                         "Translate this string to the string 'LTR' in left-to-right"
                         " languages or to 'RTL' in right-to-left languages (such as Hebrew"
                         " and Arabic) to get proper widget layout.") == QLatin1String("RTL"));
}


QGuiApplication::QGuiApplication(int &argc, char **argv, int flags)
    : QCoreApplication(*new QGuiApplicationPrivate(argc, argv, flags))
{
    d_func()->init();

    QCoreApplicationPrivate::eventDispatcher->startingUp();
}

QGuiApplication::QGuiApplication(QGuiApplicationPrivate &p)
    : QCoreApplication(p)
{
    d_func()->init();
}

QGuiApplication::~QGuiApplication()
{
    Q_D(QGuiApplication);
    // flush clipboard contents
    if (QGuiApplicationPrivate::qt_clipboard) {
        QEvent event(QEvent::Clipboard);
        QGuiApplication::sendEvent(QGuiApplicationPrivate::qt_clipboard, &event);
    }

    d->eventDispatcher->closingDown();
    d->eventDispatcher = 0;

    delete QGuiApplicationPrivate::qt_clipboard;
    QGuiApplicationPrivate::qt_clipboard = 0;

#ifndef QT_NO_CURSOR
    d->cursor_list.clear();
#endif
}

QGuiApplicationPrivate::QGuiApplicationPrivate(int &argc, char **argv, int flags)
    : QCoreApplicationPrivate(argc, argv, flags)
{
    self = this;
}

QWindowList QGuiApplication::topLevelWindows()
{
    return QGuiApplicationPrivate::window_list;
}

static void init_platform(const QString &name, const QString &platformPluginPath)
{
    QGuiApplicationPrivate::platform_integration = QPlatformIntegrationFactory::create(name, platformPluginPath);
    if (!QGuiApplicationPrivate::platform_integration) {
        QStringList keys = QPlatformIntegrationFactory::keys(platformPluginPath);
        QString fatalMessage =
            QString::fromLatin1("Failed to load platform plugin \"%1\". Available platforms are: \n").arg(name);
        foreach(QString key, keys) {
            fatalMessage.append(key + QString::fromLatin1("\n"));
        }
        qFatal("%s", fatalMessage.toLocal8Bit().constData());

    }

}

static void init_plugins(const QList<QByteArray> pluginList)
{
    for (int i = 0; i < pluginList.count(); ++i) {
        QByteArray pluginSpec = pluginList.at(i);
        qDebug() << "init_plugins" << i << pluginSpec;
        int colonPos = pluginSpec.indexOf(':');
        QObject *plugin;
        if (colonPos < 0)
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec), QString());
        else
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec.mid(0, colonPos)),
                                                   QLatin1String(pluginSpec.mid(colonPos+1)));
        qDebug() << "	created" << plugin;
    }
}

void QGuiApplicationPrivate::createEventDispatcher()
{
    Q_Q(QGuiApplication);
#if !defined(QT_NO_GLIB)
    if (qgetenv("QT_NO_GLIB").isEmpty() && QEventDispatcherGlib::versionSupported())
        eventDispatcher = new QPAEventDispatcherGlib(q);
    else
#endif
    eventDispatcher = new QEventDispatcherQPA(q);
}

void QGuiApplicationPrivate::init()
{
    QList<QByteArray> pluginList;
    QString platformPluginPath = QLatin1String(qgetenv("QT_QPA_PLATFORM_PLUGIN_PATH"));
    QByteArray platformName;
#ifdef QT_QPA_DEFAULT_PLATFORM_NAME
    platformName = QT_QPA_DEFAULT_PLATFORM_NAME;
#endif
    QByteArray platformNameEnv = qgetenv("QT_QPA_PLATFORM");
    if (!platformNameEnv.isEmpty()) {
        platformName = platformNameEnv;
    }

    // Get command line params

    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg = argv[i];
        if (arg == "-platformpluginpath") {
            if (++i < argc)
                platformPluginPath = QLatin1String(argv[i]);
        } else if (arg == "-platform") {
            if (++i < argc)
                platformName = argv[i];
        } else if (arg == "-plugin") {
            if (++i < argc)
                pluginList << argv[i];
        } else if (arg == "-reverse") {
            force_reverse = true;
            QGuiApplication::setLayoutDirection(Qt::RightToLeft);
        } else {
            argv[j++] = argv[i];
        }
    }

    argv[j] = 0;
    argc = j;

#if 0
    QByteArray pluginEnv = qgetenv("QT_QPA_PLUGINS");
    if (!pluginEnv.isEmpty()) {
        pluginList.append(pluginEnv.split(';'));
    }
#endif

    init_platform(QLatin1String(platformName), platformPluginPath);
    init_plugins(pluginList);

    QFont::initialize();

    is_app_running = true;
}

QGuiApplicationPrivate::~QGuiApplicationPrivate()
{
    delete platform_integration;
    platform_integration = 0;

    is_app_closing = true;
    is_app_running = false;

    QFont::cleanup();

    layout_direction = Qt::LeftToRight;
}

#if 0
#ifndef QT_NO_CURSOR
QCursor *overrideCursor();
void setOverrideCursor(const QCursor &);
void changeOverrideCursor(const QCursor &);
void restoreOverrideCursor();
#endif

static QFont font();
static QFont font(const QWidget*);
static QFont font(const char *className);
static void setFont(const QFont &, const char* className = 0);
static QFontMetrics fontMetrics();

#ifndef QT_NO_CLIPBOARD
static QClipboard *clipboard();
#endif
#endif

Qt::KeyboardModifiers QGuiApplication::keyboardModifiers()
{
    return QGuiApplicationPrivate::modifier_buttons;
}

Qt::MouseButtons QGuiApplication::mouseButtons()
{
    return QGuiApplicationPrivate::mouse_buttons;
}

void QGuiApplication::setDoubleClickInterval(int ms)
{
    QGuiApplicationPrivate::mouse_double_click_time = ms;
}

int QGuiApplication::doubleClickInterval()
{
    return QGuiApplicationPrivate::mouse_double_click_time;
}

void QGuiApplication::setKeyboardInputInterval(int ms)
{
    QGuiApplicationPrivate::keyboard_input_time = ms;
}

int QGuiApplication::keyboardInputInterval()
{
    return QGuiApplicationPrivate::keyboard_input_time;
}

QPlatformNativeInterface *QGuiApplication::platformNativeInterface()
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi->nativeInterface();
}

int QGuiApplication::exec()
{
    return QCoreApplication::exec();
}

bool QGuiApplication::notify(QObject *object, QEvent *event)
{
    return QCoreApplication::notify(object, event);
}

bool QGuiApplication::event(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        setLayoutDirection(qt_detectRTLLanguage()?Qt::RightToLeft:Qt::LeftToRight);
    }
    return QCoreApplication::event(e);
}

bool QGuiApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    return QCoreApplication::compressEvent(event, receiver, postedEvents);
}

void QGuiApplicationPrivate::processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)
{
    switch(e->type) {
    case QWindowSystemInterfacePrivate::Mouse:
        QGuiApplicationPrivate::processMouseEvent(static_cast<QWindowSystemInterfacePrivate::MouseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Wheel:
        QGuiApplicationPrivate::processWheelEvent(static_cast<QWindowSystemInterfacePrivate::WheelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Key:
        QGuiApplicationPrivate::processKeyEvent(static_cast<QWindowSystemInterfacePrivate::KeyEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Touch:
        QGuiApplicationPrivate::processTouchEvent(static_cast<QWindowSystemInterfacePrivate::TouchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::GeometryChange:
        QGuiApplicationPrivate::processGeometryChangeEvent(static_cast<QWindowSystemInterfacePrivate::GeometryChangeEvent*>(e));
        break;
    case QWindowSystemInterfacePrivate::Enter:
        QGuiApplicationPrivate::processEnterEvent(static_cast<QWindowSystemInterfacePrivate::EnterEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Leave:
        QGuiApplicationPrivate::processLeaveEvent(static_cast<QWindowSystemInterfacePrivate::LeaveEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ActivatedWindow:
        QGuiApplicationPrivate::processActivatedEvent(static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Close:
        QGuiApplicationPrivate::processCloseEvent(
                static_cast<QWindowSystemInterfacePrivate::CloseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenCountChange:
        QGuiApplicationPrivate::reportScreenCount(
                static_cast<QWindowSystemInterfacePrivate::ScreenCountEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenGeometry:
        QGuiApplicationPrivate::reportGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenAvailableGeometry:
        QGuiApplicationPrivate::reportAvailableGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *>(e));
        break;
    default:
        qWarning() << "Unknown user input event type:" << e->type;
        break;
    }
}

void QGuiApplicationPrivate::processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e)
{
    QEvent::Type type;
    // move first
    Qt::MouseButtons stateChange = e->buttons ^ buttons;
    if (e->globalPos != QPoint(qt_last_x, qt_last_y) && (stateChange != Qt::NoButton)) {
        QWindowSystemInterfacePrivate::MouseEvent * newMouseEvent =
                new QWindowSystemInterfacePrivate::MouseEvent(e->window.data(), e->timestamp, e->localPos, e->globalPos, e->buttons);
        QWindowSystemInterfacePrivate::windowSystemEventQueue.prepend(newMouseEvent); // just in case the move triggers a new event loop
        stateChange = Qt::NoButton;
    }

    QWindow *window = e->window.data();

    QPoint localPoint = e->localPos;
    QPoint globalPoint = e->globalPos;

    Qt::MouseButton button = Qt::NoButton;

    if (qt_last_x != globalPoint.x() || qt_last_y != globalPoint.y()) {
        type = QEvent::MouseMove;
        qt_last_x = globalPoint.x();
        qt_last_y = globalPoint.y();
        if (qAbs(globalPoint.x() - mousePressX) > mouse_double_click_distance||
            qAbs(globalPoint.y() - mousePressY) > mouse_double_click_distance)
            mousePressButton = Qt::NoButton;
    }
    else { // check to see if a new button has been pressed/released
        for (int check = Qt::LeftButton;
             check <= Qt::XButton2;
             check = check << 1) {
            if (check & stateChange) {
                button = Qt::MouseButton(check);
                break;
            }
        }
        if (button == Qt::NoButton) {
            // Ignore mouse events that don't change the current state
            return;
        }
        buttons = e->buttons;
        if (button & e->buttons) {
            if ((e->timestamp - mousePressTime) < static_cast<ulong>(QGuiApplication::doubleClickInterval()) && button == mousePressButton) {
                type = QEvent::MouseButtonDblClick;
                mousePressButton = Qt::NoButton;
            }
            else {
                type = QEvent::MouseButtonPress;
                mousePressTime = e->timestamp;
                mousePressButton = button;
                mousePressX = qt_last_x;
                mousePressY = qt_last_y;
            }
        }
        else
            type = QEvent::MouseButtonRelease;
    }


    if (window) {
        QMouseEvent ev(type, localPoint, globalPoint, button, buttons, QGuiApplication::keyboardModifiers());
        QGuiApplication::sendSpontaneousEvent(window, &ev);
        return;
    }
}


//### there's a lot of duplicated logic here -- refactoring required!

void QGuiApplicationPrivate::processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e)
{
    QPoint globalPoint = e->globalPos;

    qt_last_x = globalPoint.x();
    qt_last_y = globalPoint.y();

    QWindow *window = e->window.data();

    if (window) {
         QWheelEvent ev(e->localPos, e->globalPos, e->delta, buttons, QGuiApplication::keyboardModifiers(),
                        e->orient);
         QGuiApplication::sendSpontaneousEvent(window, &ev);
         return;
     }
}



// Remember, Qt convention is:  keyboard state is state *before*

void QGuiApplicationPrivate::processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e)
{
    QWindow *window = e->window.data();
    if (!window)
        return;

    QObject *target = window;

    if (e->nativeScanCode || e->nativeVirtualKey || e->nativeModifiers) {
        QKeyEventEx ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount,
                       e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers);
        QGuiApplication::sendSpontaneousEvent(target, &ev);
    } else {
        QKeyEvent ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount);
        QGuiApplication::sendSpontaneousEvent(target, &ev);
    }
}

void QGuiApplicationPrivate::processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e)
{
    QEvent event(QEvent::Enter);
    QCoreApplication::sendSpontaneousEvent(e->enter.data(), &event);
}

void QGuiApplicationPrivate::processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e)
{
    QEvent event(QEvent::Leave);
    QCoreApplication::sendSpontaneousEvent(e->leave.data(), &event);
}

void QGuiApplicationPrivate::processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *)
{
//    QGuiApplication::setActiveWindow(e->activated.data());
}

void QGuiApplicationPrivate::processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e)
{
    if (e->tlw.isNull())
       return;

    QWindow *window = e->tlw.data();
    if (!window)
        return;

    QRect newRect = e->newGeometry;
    QRect cr = window->geometry();

    bool isResize = cr.size() != newRect.size();
    bool isMove = cr.topLeft() != newRect.topLeft();

    window->d_func()->geometry = newRect;

    if (isResize) {
        QResizeEvent e(newRect.size(), cr.size());
        QGuiApplication::sendSpontaneousEvent(window, &e);
    }

    if (isMove) {
        //### frame geometry
        QMoveEvent e(newRect.topLeft(), cr.topLeft());
        QGuiApplication::sendSpontaneousEvent(window, &e);
    }
}

void QGuiApplicationPrivate::processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e)
{
    if (e->window.isNull())
        return;

    QCloseEvent event;
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &event);
}

void QGuiApplicationPrivate::processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *)
{
//    translateRawTouchEvent(e->widget.data(), e->devType, e->points);
}

void QGuiApplicationPrivate::reportScreenCount(QWindowSystemInterfacePrivate::ScreenCountEvent *)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    //QGuiApplication::desktop()->d_func()->updateScreenList();
    // signal anything listening for creation or deletion of screens
    //QDesktopWidget *desktop = QGuiApplication::desktop();
    //emit desktop->screenCountChanged(e->count);
}

void QGuiApplicationPrivate::reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;
}

void QGuiApplicationPrivate::reportAvailableGeometryChange(
        QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;
}

#ifndef QT_NO_CLIPBOARD
QClipboard * QGuiApplication::clipboard()
{
    if (QGuiApplicationPrivate::qt_clipboard == 0) {
        if (!qApp) {
            qWarning("QGuiApplication: Must construct a QGuiApplication before accessing a QClipboard");
            return 0;
        }
        QGuiApplicationPrivate::qt_clipboard = new QClipboard(0);
    }
    return QGuiApplicationPrivate::qt_clipboard;
}
#endif

QFont QGuiApplication::font()
{
    QMutexLocker locker(applicationFontMutex());
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font = new QFont(QLatin1String("Helvetica"));
    return *QGuiApplicationPrivate::app_font;
}

void QGuiApplication::setFont(const QFont &font)
{
    QMutexLocker locker(applicationFontMutex());
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font = new QFont(font);
    else
        *QGuiApplicationPrivate::app_font = font;
}

/*!
    \fn bool QGuiApplication::isRightToLeft()

    Returns true if the application's layout direction is
    Qt::RightToLeft; otherwise returns false.

    \sa layoutDirection(), isLeftToRight()
*/

/*!
    \fn bool QGuiApplication::isLeftToRight()

    Returns true if the application's layout direction is
    Qt::LeftToRight; otherwise returns false.

    \sa layoutDirection(), isRightToLeft()
*/

void QGuiApplicationPrivate::notifyLayoutDirectionChange()
{
}

/*!
    \property QGuiApplication::layoutDirection
    \brief the default layout direction for this application

    On system start-up, the default layout direction depends on the
    application's language.

    \sa QWidget::layoutDirection, isLeftToRight(), isRightToLeft()
 */

void QGuiApplication::setLayoutDirection(Qt::LayoutDirection direction)
{
    if (layout_direction == direction || direction == Qt::LayoutDirectionAuto)
        return;

    layout_direction = direction;

    QGuiApplicationPrivate::self->notifyLayoutDirectionChange();
}

Qt::LayoutDirection QGuiApplication::layoutDirection()
{
    return layout_direction;
}

/*!
    \fn QCursor *QGuiApplication::overrideCursor()

    Returns the active application override cursor.

    This function returns 0 if no application cursor has been defined (i.e. the
    internal cursor stack is empty).

    \sa setOverrideCursor(), restoreOverrideCursor()
*/
#ifndef QT_NO_CURSOR
QCursor *QGuiApplication::overrideCursor()
{
    return qGuiApp->d_func()->cursor_list.isEmpty() ? 0 : &qGuiApp->d_func()->cursor_list.first();
}

/*!
    Changes the currently active application override cursor to \a cursor.

    This function has no effect if setOverrideCursor() was not called.

    \sa setOverrideCursor(), overrideCursor(), restoreOverrideCursor(),
    QWidget::setCursor()
 */
void QGuiApplication::changeOverrideCursor(const QCursor &cursor)
{
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    setOverrideCursor(cursor);
}
#endif

/*!
    \fn void QGuiApplication::setOverrideCursor(const QCursor &cursor, bool replace)

    Use changeOverrideCursor(\a cursor) (if \a replace is true) or
    setOverrideCursor(\a cursor) (if \a replace is false).
*/

#ifndef QT_NO_CURSOR
void QGuiApplication::setOverrideCursor(const QCursor &cursor)
{
    qGuiApp->d_func()->cursor_list.prepend(cursor);
    qt_qpa_set_cursor(0, false);
}

void QGuiApplication::restoreOverrideCursor()
{
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    qt_qpa_set_cursor(0, false);
}
#endif// QT_NO_CURSOR


// Returns the current platform used by keyBindings
uint QGuiApplicationPrivate::currentKeyPlatform()
{
    uint platform = KB_Win;
#ifdef Q_WS_MAC
    platform = KB_Mac;
#elif defined Q_WS_X11
    platform = KB_X11;
    // ## TODO: detect these
#if 0
    if (X11->desktopEnvironment == DE_KDE)
        platform |= KB_KDE;
    if (X11->desktopEnvironment == DE_GNOME)
        platform |= KB_Gnome;
    if (X11->desktopEnvironment == DE_CDE)
        platform |= KB_CDE;
#endif
#endif
    return platform;
}

/*!
    \since 4.2

    Returns the current keyboard input locale.
*/
QLocale QGuiApplication::keyboardInputLocale()
{
    if (!QGuiApplicationPrivate::checkInstance("keyboardInputLocale"))
        return QLocale::c();
    return qt_keymapper_private()->keyboardInputLocale;
}

/*!
    \since 4.2

    Returns the current keyboard input direction.
*/
Qt::LayoutDirection QGuiApplication::keyboardInputDirection()
{
    if (!QGuiApplicationPrivate::checkInstance("keyboardInputDirection"))
        return Qt::LeftToRight;
    return qt_keymapper_private()->keyboardInputDirection;
}

/*!
    \since 4.5
    \fn void QGuiApplication::fontDatabaseChanged()

    This signal is emitted when application fonts are loaded or removed.

    \sa QFontDatabase::addApplicationFont(),
    QFontDatabase::addApplicationFontFromData(),
    QFontDatabase::removeAllApplicationFonts(),
    QFontDatabase::removeApplicationFont()
*/

// These pixmaps approximate the images in the Windows User Interface Guidelines.

// XPM

static const char * const move_xpm[] = {
"11 20 3 1",
".        c None",
#if defined(Q_WS_WIN)
"a        c #000000",
"X        c #FFFFFF", // Windows cursor is traditionally white
#else
"a        c #FFFFFF",
"X        c #000000", // X11 cursor is traditionally black
#endif
"aa.........",
"aXa........",
"aXXa.......",
"aXXXa......",
"aXXXXa.....",
"aXXXXXa....",
"aXXXXXXa...",
"aXXXXXXXa..",
"aXXXXXXXXa.",
"aXXXXXXXXXa",
"aXXXXXXaaaa",
"aXXXaXXa...",
"aXXaaXXa...",
"aXa..aXXa..",
"aa...aXXa..",
"a.....aXXa.",
"......aXXa.",
".......aXXa",
".......aXXa",
"........aa."};

#ifdef Q_WS_WIN
/* XPM */
static const char * const ignore_xpm[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
"aa......................",
"aXa.....................",
"aXXa....................",
"aXXXa...................",
"aXXXXa..................",
"aXXXXXa.................",
"aXXXXXXa................",
"aXXXXXXXa...............",
"aXXXXXXXXa..............",
"aXXXXXXXXXa.............",
"aXXXXXXaaaa.............",
"aXXXaXXa................",
"aXXaaXXa................",
"aXa..aXXa...............",
"aa...aXXa...............",
"a.....aXXa..............",
"......aXXa.....XXXX.....",
".......aXXa..XXaaaaXX...",
".......aXXa.XaaaaaaaaX..",
"........aa.XaaaXXXXaaaX.",
"...........XaaaaX..XaaX.",
"..........XaaXaaaX..XaaX",
"..........XaaXXaaaX.XaaX",
"..........XaaX.XaaaXXaaX",
"..........XaaX..XaaaXaaX",
"...........XaaX..XaaaaX.",
"...........XaaaXXXXaaaX.",
"............XaaaaaaaaX..",
".............XXaaaaXX...",
"...............XXXX....."};
#endif

/* XPM */
static const char * const copy_xpm[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
#if defined(Q_WS_WIN) // Windows cursor is traditionally white
"aa......................",
"aXa.....................",
"aXXa....................",
"aXXXa...................",
"aXXXXa..................",
"aXXXXXa.................",
"aXXXXXXa................",
"aXXXXXXXa...............",
"aXXXXXXXXa..............",
"aXXXXXXXXXa.............",
"aXXXXXXaaaa.............",
"aXXXaXXa................",
"aXXaaXXa................",
"aXa..aXXa...............",
"aa...aXXa...............",
"a.....aXXa..............",
"......aXXa..............",
".......aXXa.............",
".......aXXa.............",
"........aa...aaaaaaaaaaa",
#else
"XX......................",
"XaX.....................",
"XaaX....................",
"XaaaX...................",
"XaaaaX..................",
"XaaaaaX.................",
"XaaaaaaX................",
"XaaaaaaaX...............",
"XaaaaaaaaX..............",
"XaaaaaaaaaX.............",
"XaaaaaaXXXX.............",
"XaaaXaaX................",
"XaaXXaaX................",
"XaX..XaaX...............",
"XX...XaaX...............",
"X.....XaaX..............",
"......XaaX..............",
".......XaaX.............",
".......XaaX.............",
"........XX...aaaaaaaaaaa",
#endif
".............aXXXXXXXXXa",
".............aXXXXXXXXXa",
".............aXXXXaXXXXa",
".............aXXXXaXXXXa",
".............aXXaaaaaXXa",
".............aXXXXaXXXXa",
".............aXXXXaXXXXa",
".............aXXXXXXXXXa",
".............aXXXXXXXXXa",
".............aaaaaaaaaaa"};

/* XPM */
static const char * const link_xpm[] = {
"24 30 3 1",
".        c None",
"a        c #000000",
"X        c #FFFFFF",
#if defined(Q_WS_WIN) // Windows cursor is traditionally white
"aa......................",
"aXa.....................",
"aXXa....................",
"aXXXa...................",
"aXXXXa..................",
"aXXXXXa.................",
"aXXXXXXa................",
"aXXXXXXXa...............",
"aXXXXXXXXa..............",
"aXXXXXXXXXa.............",
"aXXXXXXaaaa.............",
"aXXXaXXa................",
"aXXaaXXa................",
"aXa..aXXa...............",
"aa...aXXa...............",
"a.....aXXa..............",
"......aXXa..............",
".......aXXa.............",
".......aXXa.............",
"........aa...aaaaaaaaaaa",
#else
"XX......................",
"XaX.....................",
"XaaX....................",
"XaaaX...................",
"XaaaaX..................",
"XaaaaaX.................",
"XaaaaaaX................",
"XaaaaaaaX...............",
"XaaaaaaaaX..............",
"XaaaaaaaaaX.............",
"XaaaaaaXXXX.............",
"XaaaXaaX................",
"XaaXXaaX................",
"XaX..XaaX...............",
"XX...XaaX...............",
"X.....XaaX..............",
"......XaaX..............",
".......XaaX.............",
".......XaaX.............",
"........XX...aaaaaaaaaaa",
#endif
".............aXXXXXXXXXa",
".............aXXXaaaaXXa",
".............aXXXXaaaXXa",
".............aXXXaaaaXXa",
".............aXXaaaXaXXa",
".............aXXaaXXXXXa",
".............aXXaXXXXXXa",
".............aXXXaXXXXXa",
".............aXXXXXXXXXa",
".............aaaaaaaaaaa"};

QPixmap QGuiApplicationPrivate::getPixmapCursor(Qt::CursorShape cshape)
{
#if defined(Q_WS_X11) || defined(Q_WS_WIN)
    if (!move_cursor) {
        move_cursor = new QPixmap((const char **)move_xpm);
        copy_cursor = new QPixmap((const char **)copy_xpm);
        link_cursor = new QPixmap((const char **)link_xpm);
#ifdef Q_WS_WIN
        ignore_cursor = new QPixmap((const char **)ignore_xpm);
#endif
    }

    switch (cshape) {
    case Qt::DragMoveCursor:
        return *move_cursor;
    case Qt::DragCopyCursor:
        return *copy_cursor;
    case Qt::DragLinkCursor:
        return *link_cursor;
#ifdef Q_WS_WIN
    case Qt::ForbiddenCursor:
        return *ignore_cursor;
#endif
    default:
        break;
    }
#else
    Q_UNUSED(cshape);
#endif
    return QPixmap();
}

QT_END_NAMESPACE
