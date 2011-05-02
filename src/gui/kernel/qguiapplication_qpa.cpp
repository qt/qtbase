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

#include "qguiapplication_qpa.h"

#include "private/qguiapplication_qpa_p.h"
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
#include "private/qwindow_qpa_p.h"

#ifndef QT_NO_CLIPBOARD
#include <QtGui/QClipboard>
#endif

QT_BEGIN_NAMESPACE

Qt::MouseButtons QGuiApplicationPrivate::mouse_buttons = Qt::NoButton;
Qt::KeyboardModifiers QGuiApplicationPrivate::modifier_buttons = Qt::NoModifier;

int QGuiApplicationPrivate::keyboard_input_time = 0;
int QGuiApplicationPrivate::mouse_double_click_time = 0;

QPlatformIntegration *QGuiApplicationPrivate::platform_integration = 0;

QWidget *qt_button_down = 0; // widget got last button-down

bool QGuiApplicationPrivate::app_do_modal = false;

int qt_last_x = 0;
int qt_last_y = 0;
QPointer<QWidget> QGuiApplicationPrivate::qt_last_mouse_receiver = 0;

QWidgetList QGuiApplicationPrivate::qt_modal_stack;

Qt::MouseButtons QGuiApplicationPrivate::buttons = Qt::NoButton;
ulong QGuiApplicationPrivate::mousePressTime = 0;
Qt::MouseButton QGuiApplicationPrivate::mousePressButton = Qt::NoButton;
int QGuiApplicationPrivate::mousePressX = 0;
int QGuiApplicationPrivate::mousePressY = 0;
int QGuiApplicationPrivate::mouse_double_click_distance = 5;

QGuiApplicationPrivate *QGuiApplicationPrivate::self = 0;

#ifndef QT_NO_CLIPBOARD
QClipboard *QGuiApplicationPrivate::qt_clipboard = 0;
#endif

Q_GLOBAL_STATIC(QMutex, applicationFontMutex)
QFont *QGuiApplicationPrivate::app_font = 0;

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
        QApplication::sendEvent(QGuiApplicationPrivate::qt_clipboard, &event);
    }

    d->eventDispatcher->closingDown();
    d->eventDispatcher = 0;

    delete QGuiApplicationPrivate::qt_clipboard;
    QGuiApplicationPrivate::qt_clipboard = 0;
}

QGuiApplicationPrivate::QGuiApplicationPrivate(int &argc, char **argv, int flags)
    : QCoreApplicationPrivate(argc, argv, flags)
{
    self = this;
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
    // qDebug() << "handleMouseEvent" << tlw << ev.pos() << ev.globalPos() << hex << ev.buttons();
    static QWeakPointer<QWidget> implicit_mouse_grabber;

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

    QWidget * tlw = 0;//window ? window->widget() : 0;

    QPoint localPoint = e->localPos;
    QPoint globalPoint = e->globalPos;
    QWidget *mouseWindow = tlw;

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


    if (window && !tlw) {
        QMouseEvent ev(type, localPoint, globalPoint, button, buttons, QGuiApplication::keyboardModifiers());
        QGuiApplication::sendSpontaneousEvent(window, &ev);
        return;
    }

#if 0
    if (self->inPopupMode()) {
        //popup mouse handling is magical...
        mouseWindow = qApp->activePopupWidget();

        implicit_mouse_grabber.clear();
        //### how should popup mode and implicit mouse grab interact?

    } else if (tlw && app_do_modal && !qt_try_modal(tlw, QEvent::MouseButtonRelease) ) {
        //even if we're blocked by modality, we should deliver the mouse release event..
        //### this code is not completely correct: multiple buttons can be pressed simultaneously
        if (!(implicit_mouse_grabber && buttons == Qt::NoButton)) {
            //qDebug() << "modal blocked mouse event to" << tlw;
            return;
        }
    }
#endif

#if 0
    // find the tlw if we didn't get it from the plugin
    if (!mouseWindow) {
        mouseWindow = QGuiApplication::topLevelAt(globalPoint);
    }

    if (!mouseWindow && !implicit_mouse_grabber)
        mouseWindow = QGuiApplication::desktop();

    if (mouseWindow && mouseWindow != tlw) {
        //we did not get a sensible localPoint from the window system, so let's calculate it
        localPoint = mouseWindow->mapFromGlobal(globalPoint);
    }
#endif

    // which child should have it?
    QWidget *mouseWidget = mouseWindow;
    if (mouseWindow) {
        QWidget *w =  mouseWindow->childAt(localPoint);
        if (w) {
            mouseWidget = w;
        }
    }

    //handle implicit mouse grab
    if (type == QEvent::MouseButtonPress && !implicit_mouse_grabber) {
        implicit_mouse_grabber = mouseWidget;

        Q_ASSERT(mouseWindow);
        mouseWindow->activateWindow(); //focus
    } else if (implicit_mouse_grabber) {
        mouseWidget = implicit_mouse_grabber.data();
        mouseWindow = mouseWidget->window();
#if 0
        if (mouseWindow != tlw)
            localPoint = mouseWindow->mapFromGlobal(globalPoint);
#endif
    }

    if (!mouseWidget)
        return;

    Q_ASSERT(mouseWidget);

    //localPoint is local to mouseWindow, but it needs to be local to mouseWidget
    localPoint = mouseWidget->mapFrom(mouseWindow, localPoint);

    if (buttons == Qt::NoButton) {
        //qDebug() << "resetting mouse grabber";
        implicit_mouse_grabber.clear();
    }

#if 0
    if (mouseWidget != qt_last_mouse_receiver) {
//        dispatchEnterLeave(mouseWidget, qt_last_mouse_receiver);
        qt_last_mouse_receiver = mouseWidget;
    }
#endif

    // Remember, we might enter a modal event loop when sending the event,
    // so think carefully before adding code below this point.

    // qDebug() << "sending mouse ev." << ev.type() << localPoint << globalPoint << ev.button() << ev.buttons() << mouseWidget << "mouse grabber" << implicit_mouse_grabber;

    QMouseEvent ev(type, localPoint, globalPoint, button, buttons, QGuiApplication::keyboardModifiers());

#if 0
    QList<QWeakPointer<QPlatformCursor> > cursors = QPlatformCursorPrivate::getInstances();
    foreach (QWeakPointer<QPlatformCursor> cursor, cursors) {
        if (cursor)
            cursor.data()->pointerEvent(ev);
    }
#endif

//    int oldOpenPopupCount = openPopupCount;
    QGuiApplication::sendSpontaneousEvent(mouseWidget, &ev);

#if 0
#ifndef QT_NO_CONTEXTMENU
    if (type == QEvent::MouseButtonPress && button == Qt::RightButton && (openPopupCount == oldOpenPopupCount)) {
        QContextMenuEvent e(QContextMenuEvent::Mouse, localPoint, globalPoint, QGuiApplication::keyboardModifiers());
        QGuiApplication::sendSpontaneousEvent(mouseWidget, &e);
    }
#endif // QT_NO_CONTEXTMENU
#endif
}


//### there's a lot of duplicated logic here -- refactoring required!

void QGuiApplicationPrivate::processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e)
{
//    QPoint localPoint = ev.pos();
    QPoint globalPoint = e->globalPos;
//    bool trustLocalPoint = !!tlw; //is there something the local point can be local to?

    qt_last_x = globalPoint.x();
    qt_last_y = globalPoint.y();

    QWindow *window = e->window.data();
    if (!window)
        return;

    QWidget *mouseWidget = 0;//window ? window->widget() : 0;

     // find the tlw if we didn't get it from the plugin
#if 0
     if (!mouseWindow) {
         mouseWindow = QGuiApplication::topLevelAt(globalPoint);
     }
#endif

     if (!mouseWidget) {
         QWheelEvent ev(e->localPos, e->globalPos, e->delta, buttons, QGuiApplication::keyboardModifiers(),
                        e->orient);
         QGuiApplication::sendSpontaneousEvent(window, &ev);
         return;
     }

#if 0
     if (app_do_modal && !qt_try_modal(mouseWindow, QEvent::Wheel) ) {
         qDebug() << "modal blocked wheel event" << mouseWindow;
         return;
     }
     QPoint p = mouseWindow->mapFromGlobal(globalPoint);
     QWidget *w = mouseWindow->childAt(p);
     if (w) {
         mouseWidget = w;
         p = mouseWidget->mapFromGlobal(globalPoint);
     }
#endif

     QWheelEvent ev(e->localPos, e->globalPos, e->delta, buttons, QGuiApplication::keyboardModifiers(),
                   e->orient);
     QGuiApplication::sendSpontaneousEvent(mouseWidget, &ev);
}



// Remember, Qt convention is:  keyboard state is state *before*

void QGuiApplicationPrivate::processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e)
{
    QWindow *window = e->window.data();
    if (!window)
        return;

    QObject *target = window;//window->widget() ? static_cast<QObject *>(window->widget()) : static_cast<QObject *>(window);

#if 0
    QWidget *focusW = 0;
    if (self->inPopupMode()) {
        QWidget *popupW = qApp->activePopupWidget();
        focusW = popupW->focusWidget() ? popupW->focusWidget() : popupW;
    }
    if (!focusW)
        focusW = QGuiApplication::focusWidget();
    if (!focusW)
        focusW = window->widget();
    if (!focusW)
        focusW = QGuiApplication::activeWindow();
#endif

    //qDebug() << "handleKeyEvent" << hex << e->key() << e->modifiers() << e->text() << "widget" << focusW;

#if 0
    if (!focusW)
        return;
    if (app_do_modal && !qt_try_modal(focusW, e->keyType))
        return;
#endif

    if (e->nativeScanCode || e->nativeVirtualKey || e->nativeModifiers) {
        QKeyEventEx ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount,
                       e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers);
        QGuiApplication::sendSpontaneousEvent(target, &ev);
    } else {
        QKeyEvent ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount);
        QGuiApplication::sendSpontaneousEvent(target, &ev);
    }
}

void QGuiApplicationPrivate::processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *)
{
//    QGuiApplicationPrivate::dispatchEnterLeave(e->enter.data(),0);
//    qt_last_mouse_receiver = e->enter.data();
}

void QGuiApplicationPrivate::processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *)
{
//    QGuiApplicationPrivate::dispatchEnterLeave(0,qt_last_mouse_receiver);

#if 0
    if (e->leave.data() && !e->leave.data()->isAncestorOf(qt_last_mouse_receiver)) //(???) this should not happen
        QGuiApplicationPrivate::dispatchEnterLeave(0, e->leave.data());
#endif
    qt_last_mouse_receiver = 0;

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

    QWidget *tlw = 0;//window->widget();
    QObject *target = tlw ? static_cast<QObject *>(tlw) : static_cast<QObject *>(window);

    QRect newRect = e->newGeometry;
    QRect cr = tlw ? tlw->geometry() : window->geometry();

    bool isResize = cr.size() != newRect.size();
    bool isMove = cr.topLeft() != newRect.topLeft();

    if (tlw && !tlw->isWindow())
        return; //geo of native child widgets is controlled by lighthouse
                //so we already have sent the events; besides this new rect
                //is not mapped to parent


    if (tlw)
        tlw->data->crect = newRect;
    else
        window->d_func()->geometry = newRect;

    if (isResize) {
        QResizeEvent e(newRect.size(), cr.size());
        QGuiApplication::sendSpontaneousEvent(target, &e);
        if (tlw)
            tlw->update();
    }

    if (isMove) {
        //### frame geometry
        QMoveEvent e(newRect.topLeft(), cr.topLeft());
        QGuiApplication::sendSpontaneousEvent(target, &e);
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

#if 0
    QGuiApplication::desktop()->d_func()->updateScreenList();

    // signal anything listening for screen geometry changes
    QDesktopWidget *desktop = QGuiApplication::desktop();
    emit desktop->resized(e->index);

    // make sure maximized and fullscreen windows are updated
    QWidgetList list = QGuiApplication::topLevelWidgets();
    for (int i = list.size() - 1; i >= 0; --i) {
        QWidget *w = list.at(i);
        if (w->isFullScreen())
            w->d_func()->setFullScreenSize_helper();
        else if (w->isMaximized())
            w->d_func()->setMaxWindowState_helper();
    }
#endif
}

void QGuiApplicationPrivate::reportAvailableGeometryChange(
        QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

#if 0
    QGuiApplication::desktop()->d_func()->updateScreenList();

    // signal anything listening for screen geometry changes
    QDesktopWidget *desktop = QGuiApplication::desktop();
    emit desktop->workAreaResized(e->index);

    // make sure maximized and fullscreen windows are updated
    QWidgetList list = QGuiApplication::topLevelWidgets();
    for (int i = list.size() - 1; i >= 0; --i) {
        QWidget *w = list.at(i);
        if (w->isFullScreen())
            w->d_func()->setFullScreenSize_helper();
        else if (w->isMaximized())
            w->d_func()->setMaxWindowState_helper();
    }
#endif
}

#ifndef QT_NO_CLIPBOARD
QClipboard * QGuiApplication::clipboard()
{
    if (QGuiApplicationPrivate::qt_clipboard == 0) {
        if (!qApp) {
            qWarning("QApplication: Must construct a QApplication before accessing a QClipboard");
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

QT_END_NAMESPACE
