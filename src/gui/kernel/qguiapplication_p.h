/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGUIAPPLICATION_QPA_P_H
#define QGUIAPPLICATION_QPA_P_H

#include <QtGui/qguiapplication.h>

#include <QtCore/QPointF>
#include <QtCore/private/qcoreapplication_p.h>

#include <QtCore/private/qthread_p.h>

#include <QWindowSystemInterface>
#include "private/qwindowsysteminterface_qpa_p.h"
#include "QtGui/qplatformintegration_qpa.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QGuiApplicationPrivate : public QCoreApplicationPrivate
{
    Q_DECLARE_PUBLIC(QGuiApplication)
public:
    QGuiApplicationPrivate(int &argc, char **argv, int flags);
    ~QGuiApplicationPrivate();

    void createPlatformIntegration();
    void createEventDispatcher();
    void setEventDispatcher(QAbstractEventDispatcher *eventDispatcher);

    virtual void notifyLayoutDirectionChange();
    virtual void notifyActiveWindowChange(QWindow *previous);

    static Qt::KeyboardModifiers modifier_buttons;
    static Qt::MouseButtons mouse_buttons;

    static QPlatformIntegration *platform_integration;

    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }

    enum KeyPlatform {
        KB_Win = 1,
        KB_Mac = 2,
        KB_X11 = 4,
        KB_KDE = 8,
        KB_Gnome = 16,
        KB_CDE = 32,
        KB_S60 = 64,
        KB_All = 0xffff
    };

    static uint currentKeyPlatform();

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    { return QCoreApplication::instance()->d_func()->threadData->eventDispatcher; }

    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);
    static void processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void reportScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e);
    static void reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
    static void reportAvailableGeometryChange(QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e);

    static void processMapEvent(QWindowSystemInterfacePrivate::MapEvent *e);
    static void processUnmapEvent(QWindowSystemInterfacePrivate::UnmapEvent *e);

    static void processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e);

    static Qt::DropAction processDrag(QWindow *w, QMimeData *dropData, const QPoint &p);
    static Qt::DropAction processDrop(QWindow *w, QMimeData *dropData, const QPoint &p);

    static inline Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        if (!(alignment & Qt::AlignHorizontal_Mask))
            alignment |= Qt::AlignLeft;
        if ((alignment & Qt::AlignAbsolute) == 0 && (alignment & (Qt::AlignLeft | Qt::AlignRight))) {
            if (direction == Qt::RightToLeft)
                alignment ^= (Qt::AlignLeft | Qt::AlignRight);
            alignment |= Qt::AlignAbsolute;
        }
        return alignment;
    }

    static void emitLastWindowClosed();

    QPixmap getPixmapCursor(Qt::CursorShape cshape);

    static QGuiApplicationPrivate *instance() { return self; }

    static bool app_do_modal;

    static Qt::MouseButtons buttons;
    static ulong mousePressTime;
    static Qt::MouseButton mousePressButton;
    static int mousePressX;
    static int mousePressY;
    static int mouse_double_click_distance;
    static QPointF lastCursorPosition;

#ifndef QT_NO_CLIPBOARD
    static QClipboard *qt_clipboard;
#endif

    static QPalette *app_pal;

    static QWindowList window_list;
    static QWindow *focus_window;

#ifndef QT_NO_CURSOR
    QList<QCursor> cursor_list;
#endif
    static QList<QScreen *> screen_list;

    static QFont *app_font;

    QStyleHints *styleHints;
    QInputPanel *inputPanel;

    static bool quitOnLastWindowClosed;

    static QList<QObject *> generic_plugin_list;

private:
    void init();

    static QGuiApplicationPrivate *self;

    QMap<int, QWeakPointer<QWindow> > windowForTouchPointId;
    QMap<int, QTouchEvent::TouchPoint> appCurrentTouchPoints;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGUIAPPLICATION_QPA_P_H
