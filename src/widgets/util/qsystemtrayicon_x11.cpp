// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtwidgetsglobal.h"
#if QT_CONFIG(label)
#include "qlabel.h"
#endif
#include "qpainter.h"
#include "qpixmap.h"
#include "qbitmap.h"
#include "qevent.h"
#include "qapplication.h"
#include "qlist.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
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

#ifndef QT_NO_SYSTEMTRAYICON
QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *ev) override;
    virtual bool event(QEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void moveEvent(QMoveEvent *) override;

private:
    QSystemTrayIcon *q;
};

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *qIn)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint)
    , q(qIn)
{
    setObjectName(QStringLiteral("QSystemTrayIconSys"));
#if QT_CONFIG(tooltip)
    setToolTip(q->toolTip());
#endif
    setAttribute(Qt::WA_AlwaysShowToolTips, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    const QSize size(22, 22); // Gnome, standard size
    setGeometry(QRect(QPoint(0, 0), size));
    setMinimumSize(size);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

QRect QSystemTrayIconSys::globalGeometry() const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size());
}

void QSystemTrayIconSys::mousePressEvent(QMouseEvent *ev)
{
    QPoint globalPos = ev->globalPosition().toPoint();
#ifndef QT_NO_CONTEXTMENU
    if (ev->button() == Qt::RightButton && q->contextMenu())
        q->contextMenu()->popup(globalPos);
#else
    Q_UNUSED(globalPos);
#endif // QT_NO_CONTEXTMENU

    if (QBalloonTip::isBalloonVisible()) {
        emit q->messageClicked();
        QBalloonTip::hideBalloon();
    }

    if (ev->button() == Qt::LeftButton)
        emit q->activated(QSystemTrayIcon::Trigger);
    else if (ev->button() == Qt::RightButton)
        emit q->activated(QSystemTrayIcon::Context);
    else if (ev->button() == Qt::MiddleButton)
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
        QCoreApplication::sendEvent(q, e);
        break;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        return QCoreApplication::sendEvent(q, e);
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

class QSystemTrayWatcher: public QObject
{
    Q_OBJECT
public:
    QSystemTrayWatcher(QSystemTrayIcon *trayIcon)
        : QObject(trayIcon)
        , mTrayIcon(trayIcon)
    {
        // This code uses string-based syntax because we want to connect to a signal
        // which is defined in XCB plugin - QXcbNativeInterface::systemTrayWindowChanged().
        connect(qGuiApp->platformNativeInterface(), SIGNAL(systemTrayWindowChanged(QScreen*)),
                this, SLOT(systemTrayWindowChanged(QScreen*)));
    }

private slots:
    void systemTrayWindowChanged(QScreen *)
    {
        auto icon = static_cast<QSystemTrayIconPrivate *>(QObjectPrivate::get(mTrayIcon));
        icon->destroyIcon();
        if (icon->visible && locateSystemTray()) {
            icon->sys = new QSystemTrayIconSys(mTrayIcon);
            icon->sys->show();
        }
    }

private:
    QSystemTrayIcon *mTrayIcon = nullptr;
};
////////////////////////////////////////////////////////////////////////////

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(nullptr),
      qpa_sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon()),
      visible(false),
      trayWatcher(nullptr)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
    delete qpa_sys;
}

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);

    if (qpa_sys) {
        install_sys_qpa();
        return;
    }

    if (!sys) {
        if (!trayWatcher)
            trayWatcher = new QSystemTrayWatcher(q);

        if (locateSystemTray()) {
            sys = new QSystemTrayIconSys(q);
            sys->show();
        }
    }
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (qpa_sys)
        return qpa_sys->geometry();
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

    destroyIcon();
}

void QSystemTrayIconPrivate::destroyIcon()
{
    if (!sys)
        return;
    QBalloonTip::hideBalloon();
    sys->hide();
    delete sys;
    sys = nullptr;
}


void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (qpa_sys) {
        qpa_sys->updateIcon(icon);
        return;
    }
    if (sys)
        sys->updateIcon();
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
#if QT_CONFIG(menu)
    if (qpa_sys && menu) {
        addPlatformMenu(menu);
        qpa_sys->updateMenu(menu->platformMenu());
    }
#endif
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (qpa_sys) {
        qpa_sys->updateToolTip(toolTip);
        return;
    }
    if (!sys)
        return;
#if QT_CONFIG(tooltip)
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
    if (platform.compare("xcb"_L1, Qt::CaseInsensitive) == 0)
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
                                   const QIcon &icon, QSystemTrayIcon::MessageIcon msgIcon, int msecs)
{
    if (qpa_sys) {
        qpa_sys->showMessage(title, message, icon,
                         static_cast<QPlatformSystemTrayIcon::MessageIcon>(msgIcon), msecs);
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
