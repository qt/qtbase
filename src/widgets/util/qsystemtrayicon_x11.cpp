/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qlabel.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qbitmap.h"
#include "qevent.h"
#include "qapplication.h"
#include "qlist.h"
#include "qmenu.h"
#include "qtimer.h"
#include "qsystemtrayicon_p.h"
#include "qpaintengine.h"
#include <qwindow.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qbackingstore.h>
#include <qpa/qplatformnativeinterface.h>
#include <qdebug.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#ifndef QT_NO_SYSTEMTRAYICON
QT_BEGIN_NAMESPACE

enum {
    SYSTEM_TRAY_REQUEST_DOCK = 0,
    SYSTEM_TRAY_BEGIN_MESSAGE = 1,
    SYSTEM_TRAY_CANCEL_MESSAGE =2
};

// ### fixme (15.3.2012): The following issues need to be resolved:
// - Tracking of the actual tray window for DestroyNotify and re-creation
//   of the icons on the new window should it change (see Qt 4.X).

// Global context for the X11 system tray containing a display for the primary
// screen and a selection atom from which the tray window can be determined.
class QX11SystemTrayContext
{
public:
    QX11SystemTrayContext();
    ~QX11SystemTrayContext();

    bool isValid() const { return m_systemTraySelection != 0; }

    inline Display *display() const  { return m_display; }
    inline int screenNumber() const { return m_screenNumber; }
    Window locateSystemTray() const;

private:
    Display *m_display;
    int m_screenNumber;
    Atom m_systemTraySelection;
};

QX11SystemTrayContext::QX11SystemTrayContext() : m_display(0), m_screenNumber(0), m_systemTraySelection(0)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qWarning("%s: No screen.", Q_FUNC_INFO);
        return;
    }
    void *displayV = QGuiApplication::platformNativeInterface()->nativeResourceForScreen(QByteArrayLiteral("display"), screen);
    if (!displayV) {
        qWarning("%s: Unable to obtain X11 display of primary screen.", Q_FUNC_INFO);
        return;
    }

    m_display = static_cast<Display *>(displayV);

    const QByteArray netSysTray = "_NET_SYSTEM_TRAY_S" + QByteArray::number(m_screenNumber);
    m_systemTraySelection = XInternAtom(m_display, netSysTray.constData(), False);
    if (!m_systemTraySelection) {
        qWarning("%s: Unable to retrieve atom '%s'.", Q_FUNC_INFO, netSysTray.constData());
        return;
    }
}

Window QX11SystemTrayContext::locateSystemTray() const
{
    if (isValid())
        return XGetSelectionOwner(m_display, m_systemTraySelection);
    return 0;
}

QX11SystemTrayContext::~QX11SystemTrayContext()
{
}

Q_GLOBAL_STATIC(QX11SystemTrayContext, qX11SystemTrayContext)

// System tray widget. Could be replaced by a QWindow with
// a backing store if it did not need tooltip handling.
class QSystemTrayIconSys : public QWidget
{
public:
    explicit QSystemTrayIconSys(QSystemTrayIcon *q);

    inline void updateIcon() { update(); }
    inline QSystemTrayIcon *systemTrayIcon() const { return q; }

    QRect globalGeometry() const;

protected:
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseDoubleClickEvent(QMouseEvent *ev);
    virtual bool event(QEvent *);
    virtual void paintEvent(QPaintEvent *);

private:
    QSystemTrayIcon *q;
};

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *qIn)
    : QWidget(0, Qt::Window | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint)
    , q(qIn)
{
    setObjectName(QStringLiteral("QSystemTrayIconSys"));
    setToolTip(q->toolTip());
    QX11SystemTrayContext *context = qX11SystemTrayContext();
    Q_ASSERT(context->isValid());
    setAttribute(Qt::WA_AlwaysShowToolTips, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    const QSize size(22, 22); // Gnome, standard size
    setGeometry(QRect(QPoint(0, 0), size));
    setMinimumSize(size);
    createWinId();
    setMouseTracking(true);

    Display *display = context->display();

    // Request to be a tray window according to GNOME, NET WM Specification
    static Atom netwm_tray_atom = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", False);
    long l[5] = { CurrentTime, SYSTEM_TRAY_REQUEST_DOCK, static_cast<long>(winId()), 0, 0 };
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = context->locateSystemTray();
    ev.xclient.message_type = netwm_tray_atom;
    ev.xclient.format = 32;
    memcpy((char *)&ev.xclient.data, (const char *) l, sizeof(l));
    XSendEvent(display, ev.xclient.window, False, 0, &ev);
    show();
}

QRect QSystemTrayIconSys::globalGeometry() const
{
    QX11SystemTrayContext *context = qX11SystemTrayContext();
    ::Window dummy;
    int x, y, rootX, rootY;
    unsigned int width, height, border, depth;
    // Use X11 API since we are parented on the tray, about which the QWindow does not know.
    XGetGeometry(context->display(), winId(), &dummy, &x, &y, &width, &height, &border, &depth);
    XTranslateCoordinates(context->display(), winId(),
                          XRootWindow(context->display(), context->screenNumber()),
                          x, y, &rootX, &rootY, &dummy);
    return QRect(QPoint(rootX, rootY), QSize(width, height));
}


void QSystemTrayIconSys::mousePressEvent(QMouseEvent *ev)
{
    QPoint globalPos = ev->globalPos();
#ifndef QT_NO_CONTEXTMENU
    if (ev->button() == Qt::RightButton && q->contextMenu())
        q->contextMenu()->popup(globalPos);
#endif

    if (QBalloonTip::isBalloonVisible()) {
        emit q->messageClicked();
        QBalloonTip::hideBalloon();
    }

    if (ev->button() == Qt::LeftButton)
        emit q->activated(QSystemTrayIcon::Trigger);
    else if (ev->button() == Qt::RightButton)
        emit q->activated(QSystemTrayIcon::Context);
    else if (ev->button() == Qt::MidButton)
        emit q->activated(QSystemTrayIcon::MiddleClick);
}

void QSystemTrayIconSys::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit q->activated(QSystemTrayIcon::DoubleClick);
}

bool QSystemTrayIconSys::event(QEvent *e)
{
    switch (e->type()) {
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        return QApplication::sendEvent(q, e);
#endif
    default:
        break;
    }
    return QWidget::event(e);
}

void QSystemTrayIconSys::paintEvent(QPaintEvent *)
{
    // Note: Transparent pixels require a particular Visual which XCB
    // currently does not support yet.
    const QRect rect(QPoint(0, 0), geometry().size());
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    q->icon().paint(&painter, rect);
}

////////////////////////////////////////////////////////////////////////////

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(0),
      visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
}

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);
    if (!sys && qX11SystemTrayContext()->isValid())
        sys = new QSystemTrayIconSys(q);
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (!sys)
        return QRect();
    return sys->globalGeometry();
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (!sys)
        return;
    QBalloonTip::hideBalloon();
    sys->hide(); // this should do the trick, but...
    delete sys; // wm may resize system tray only for DestroyEvents
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (sys)
        sys->updateIcon();
}

void QSystemTrayIconPrivate::updateMenu_sys()
{

}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (!sys)
        return;
#ifndef QT_NO_TOOLTIP
    sys->setToolTip(toolTip);
#endif
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    const QString platform = QGuiApplication::platformName();
    if (platform.compare(QStringLiteral("xcb"), Qt::CaseInsensitive) == 0)
       return qX11SystemTrayContext()->locateSystemTray() != None;
    return false;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return true;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &message, const QString &title,
                                   QSystemTrayIcon::MessageIcon icon, int msecs)
{
    if (!sys)
        return;
    const QPoint g = sys->globalGeometry().topLeft();
    QBalloonTip::showBalloon(icon, message, title, sys->systemTrayIcon(),
                             QPoint(g.x() + sys->width()/2, g.y() + sys->height()/2),
                             msecs);
}

QT_END_NAMESPACE
#endif //QT_NO_SYSTEMTRAYICON
