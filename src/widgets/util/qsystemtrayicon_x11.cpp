/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <qpa/qplatformsystemtrayicon.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <qdebug.h>

#include <QtPlatformHeaders/qxcbwindowfunctions.h>
#include <QtPlatformHeaders/qxcbintegrationfunctions.h>
#ifndef QT_NO_SYSTEMTRAYICON
QT_BEGIN_NAMESPACE

static inline unsigned long locateSystemTray()
{
    return (unsigned long)QGuiApplication::platformNativeInterface()->nativeResourceForScreen(QByteArrayLiteral("traywindow"), QGuiApplication::primaryScreen());
}

// System tray widget. Could be replaced by a QWindow with
// a backing store if it did not need tooltip handling.
class QSystemTrayIconSys : public QWidget
{
    Q_OBJECT
public:
    explicit QSystemTrayIconSys(QSystemTrayIcon *q);

    inline void updateIcon() { update(); }
    inline QSystemTrayIcon *systemTrayIcon() const { return q; }

    QRect globalGeometry() const;

protected:
    virtual void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    virtual void moveEvent(QMoveEvent *) Q_DECL_OVERRIDE;

private slots:
    void systemTrayWindowChanged(QScreen *screen);

private:
    bool addToTray();

    QSystemTrayIcon *q;
    QPixmap background;
};

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *qIn)
    : QWidget(0, Qt::Window | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint)
    , q(qIn)
{
    setObjectName(QStringLiteral("QSystemTrayIconSys"));
    setToolTip(q->toolTip());
    setAttribute(Qt::WA_AlwaysShowToolTips, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    const QSize size(22, 22); // Gnome, standard size
    setGeometry(QRect(QPoint(0, 0), size));
    setMinimumSize(size);

    // We need two different behaviors depending on whether the X11 visual for the system tray
    // (a) exists and (b) supports an alpha channel, i.e. is 32 bits.
    // If we have a visual that has an alpha channel, we can paint this widget with a transparent
    // background and it will work.
    // However, if there's no alpha channel visual, in order for transparent tray icons to work,
    // we do not have a transparent background on the widget, but set the BackPixmap property of our
    // window to ParentRelative (so that it inherits the background of its X11 parent window), call
    // xcb_clear_region before painting (so that the inherited background is visible) and then grab
    // the just-drawn background from the X11 server.
    bool hasAlphaChannel = QXcbIntegrationFunctions::xEmbedSystemTrayVisualHasAlphaChannel();
    setAttribute(Qt::WA_TranslucentBackground, hasAlphaChannel);
    if (!hasAlphaChannel) {
        createWinId();
        QXcbWindowFunctions::setParentRelativeBackPixmap(windowHandle());

        // XXX: This is actually required, but breaks things ("QWidget::paintEngine: Should no
        // longer be called"). Why is this needed? When the widget is drawn, we use tricks to grab
        // the tray icon's background from the server. If the tray icon isn't visible (because
        // another window is on top of it), the trick fails and instead uses the content of that
        // other window as the background.
        // setAttribute(Qt::WA_PaintOnScreen);
    }

    addToTray();
}

bool QSystemTrayIconSys::addToTray()
{
    if (!locateSystemTray())
        return false;

    createWinId();
    setMouseTracking(true);

    if (!QXcbWindowFunctions::requestSystemTrayWindowDock(windowHandle()))
        return false;

    if (!background.isNull())
        background = QPixmap();
    show();
    return true;
}

void QSystemTrayIconSys::systemTrayWindowChanged(QScreen *)
{
    if (locateSystemTray()) {
        addToTray();
    } else {
        QBalloonTip::hideBalloon();
        hide(); // still no luck
        destroy();
    }
}

QRect QSystemTrayIconSys::globalGeometry() const
{
    return QXcbWindowFunctions::systemTrayWindowGlobalGeometry(windowHandle());
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
    case QEvent::ToolTip:
        QApplication::sendEvent(q, e);
        break;
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
    const QRect rect(QPoint(0, 0), geometry().size());
    QPainter painter(this);

    // If we have Qt::WA_TranslucentBackground set, during widget creation
    // we detected the systray visual supported an alpha channel
    if (testAttribute(Qt::WA_TranslucentBackground)) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect, Qt::transparent);
    } else {
        // clearRegion() was called on XEMBED_EMBEDDED_NOTIFY, so we hope that got done by now.
        // Grab the tray background pixmap, before rendering the icon for the first time.
        if (background.isNull()) {
            background = QGuiApplication::primaryScreen()->grabWindow(winId(),
                                0, 0, rect.size().width(), rect.size().height());
        }
        // Then paint over the icon area with the background before compositing the icon on top.
        painter.drawPixmap(QPoint(0, 0), background);
    }
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    q->icon().paint(&painter, rect);
}

void QSystemTrayIconSys::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    if (QBalloonTip::isBalloonVisible())
        QBalloonTip::updateBalloonPosition(globalGeometry().center());
}

void QSystemTrayIconSys::resizeEvent(QResizeEvent *event)
{
    update();
    QWidget::resizeEvent(event);
    if (QBalloonTip::isBalloonVisible())
        QBalloonTip::updateBalloonPosition(globalGeometry().center());
}
////////////////////////////////////////////////////////////////////////////

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(0),
      qpa_sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon()),
      visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
    delete qpa_sys;
}

void QSystemTrayIconPrivate::install_sys()
{
    if (qpa_sys) {
        install_sys_qpa();
        return;
    }
    Q_Q(QSystemTrayIcon);
    if (!sys && locateSystemTray()) {
        sys = new QSystemTrayIconSys(q);
        QObject::connect(QGuiApplication::platformNativeInterface(), SIGNAL(systemTrayWindowChanged(QScreen*)),
                         sys, SLOT(systemTrayWindowChanged(QScreen*)));
    }
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (qpa_sys)
        return geometry_sys_qpa();
    if (!sys)
        return QRect();
    return sys->globalGeometry();
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (qpa_sys) {
        remove_sys_qpa();
        return;
    }
    if (!sys)
        return;
    QBalloonTip::hideBalloon();
    sys->hide(); // this should do the trick, but...
    delete sys; // wm may resize system tray only for DestroyEvents
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (qpa_sys) {
        updateIcon_sys_qpa();
        return;
    }
    if (sys)
        sys->updateIcon();
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
    if (qpa_sys)
        updateMenu_sys_qpa();
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (qpa_sys) {
        updateToolTip_sys_qpa();
        return;
    }
    if (!sys)
        return;
#ifndef QT_NO_TOOLTIP
    sys->setToolTip(toolTip);
#endif
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys && sys->isSystemTrayAvailable())
        return true;

    // no QPlatformSystemTrayIcon so fall back to default xcb platform behavior
    const QString platform = QGuiApplication::platformName();
    if (platform.compare(QLatin1String("xcb"), Qt::CaseInsensitive) == 0)
       return locateSystemTray();
    return false;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->supportsMessages();

    // no QPlatformSystemTrayIcon so fall back to default xcb platform behavior
    return true;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title, const QString &message,
                                   QSystemTrayIcon::MessageIcon icon, int msecs)
{
    if (qpa_sys) {
        showMessage_sys_qpa(title, message, icon, msecs);
        return;
    }
    if (!sys)
        return;
    QBalloonTip::showBalloon(icon, title, message, sys->systemTrayIcon(),
                             sys->globalGeometry().center(),
                             msecs);
}

QT_END_NAMESPACE

#include "qsystemtrayicon_x11.moc"

#endif //QT_NO_SYSTEMTRAYICON
