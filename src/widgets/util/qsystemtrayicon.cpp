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

#include "qsystemtrayicon.h"
#include "qsystemtrayicon_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include "qlist.h"
#include "qevent.h"
#include "qpoint.h"
#if QT_CONFIG(label)
#include "qlabel.h"
#include "private/qlabel_p.h"
#endif
#if QT_CONFIG(pushbutton)
#include "qpushbutton.h"
#endif
#include "qpainterpath.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qgridlayout.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#include <private/qdesktopwidget_p.h>
#include "qbitmap.h"

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

static QIcon messageIcon2qIcon(QSystemTrayIcon::MessageIcon icon)
{
    QStyle::StandardPixmap stdIcon = QStyle::SP_CustomBase; // silence gcc 4.9.0 about uninited variable
    switch (icon) {
    case QSystemTrayIcon::Information:
        stdIcon = QStyle::SP_MessageBoxInformation;
        break;
    case QSystemTrayIcon::Warning:
        stdIcon = QStyle::SP_MessageBoxWarning;
        break;
    case QSystemTrayIcon::Critical:
        stdIcon = QStyle::SP_MessageBoxCritical;
        break;
    case QSystemTrayIcon::NoIcon:
        return QIcon();
    }
    return QApplication::style()->standardIcon(stdIcon);
}

/*!
    \class QSystemTrayIcon
    \brief The QSystemTrayIcon class provides an icon for an application in the system tray.
    \since 4.2
    \ingroup desktop
    \inmodule QtWidgets

    Modern operating systems usually provide a special area on the desktop,
    called the \e{system tray} or \e{notification area}, where long-running
    applications can display icons and short messages.

    \image system-tray.png The system tray on Windows XP.

    The QSystemTrayIcon class can be used on the following platforms:

    \list
    \li All supported versions of Windows.
    \li All window managers and independent tray implementations for X11 that implement the
       \l{http://standards.freedesktop.org/systemtray-spec/systemtray-spec-0.2.html freedesktop.org}
       XEmbed system tray specification.
    \li All X11 desktop environments that implement the D-Bus
       \l{http://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/StatusNotifierItem}
       specification, including recent versions of KDE and Unity.
    \li All supported versions of \macos.
    \endlist

    To check whether a system tray is present on the user's desktop,
    call the QSystemTrayIcon::isSystemTrayAvailable() static function.

    To add a system tray entry, create a QSystemTrayIcon object, call setContextMenu()
    to provide a context menu for the icon, and call show() to make it visible in the
    system tray. Status notification messages ("balloon messages") can be displayed at
    any time using showMessage().

    If the system tray is unavailable when a system tray icon is constructed, but
    becomes available later, QSystemTrayIcon will automatically add an entry for the
    application in the system tray if the icon is \l visible.

    The activated() signal is emitted when the user activates the icon.

    Only on X11, when a tooltip is requested, the QSystemTrayIcon receives a QHelpEvent
    of type QEvent::ToolTip. Additionally, the QSystemTrayIcon receives wheel events of
    type QEvent::Wheel. These are not supported on any other platform.

    \sa QDesktopServices, QDesktopWidget, {Desktop Integration}, {System Tray Icon Example}
*/

/*!
    \enum QSystemTrayIcon::MessageIcon

    This enum describes the icon that is shown when a balloon message is displayed.

    \value NoIcon      No icon is shown.
    \value Information An information icon is shown.
    \value Warning     A standard warning icon is shown.
    \value Critical    A critical warning icon is shown.

    \sa QMessageBox
*/

/*!
    Constructs a QSystemTrayIcon object with the given \a parent.

    The icon is initially invisible.

    \sa visible
*/
QSystemTrayIcon::QSystemTrayIcon(QObject *parent)
: QObject(*new QSystemTrayIconPrivate(), parent)
{
}

/*!
    Constructs a QSystemTrayIcon object with the given \a icon and \a parent.

    The icon is initially invisible.

    \sa visible
*/
QSystemTrayIcon::QSystemTrayIcon(const QIcon &icon, QObject *parent)
    : QSystemTrayIcon(parent)
{
    setIcon(icon);
}

/*!
    Removes the icon from the system tray and frees all allocated resources.
*/
QSystemTrayIcon::~QSystemTrayIcon()
{
    Q_D(QSystemTrayIcon);
    d->remove_sys();
}

#if QT_CONFIG(menu)

/*!
    Sets the specified \a menu to be the context menu for the system tray icon.

    The menu will pop up when the user requests the context menu for the system
    tray icon by clicking the mouse button.

    On \macos, this is currenly converted to a NSMenu, so the
    aboutToHide() signal is not emitted.

    \note The system tray icon does not take ownership of the menu. You must
    ensure that it is deleted at the appropriate time by, for example, creating
    the menu with a suitable parent object.
*/
void QSystemTrayIcon::setContextMenu(QMenu *menu)
{
    Q_D(QSystemTrayIcon);
    QMenu *oldMenu = d->menu.data();
    d->menu = menu;
    d->updateMenu_sys();
    if (oldMenu != menu && d->qpa_sys) {
        // Show the QMenu-based menu for QPA plugins that do not provide native menus
        if (oldMenu && !oldMenu->platformMenu())
            QObject::disconnect(d->qpa_sys, &QPlatformSystemTrayIcon::contextMenuRequested, menu, nullptr);
        if (menu && !menu->platformMenu()) {
            QObject::connect(d->qpa_sys, &QPlatformSystemTrayIcon::contextMenuRequested,
                             menu,
                             [menu](QPoint globalNativePos, const QPlatformScreen *platformScreen)
            {
                QScreen *screen = platformScreen ? platformScreen->screen() : nullptr;
                menu->popup(QHighDpi::fromNativePixels(globalNativePos, screen), nullptr);
            });
        }
    }
}

/*!
    Returns the current context menu for the system tray entry.
*/
QMenu* QSystemTrayIcon::contextMenu() const
{
    Q_D(const QSystemTrayIcon);
    return d->menu;
}

#endif // QT_CONFIG(menu)

/*!
    \property QSystemTrayIcon::icon
    \brief the system tray icon

    On Windows, the system tray icon size is 16x16; on X11, the preferred size is
    22x22. The icon will be scaled to the appropriate size as necessary.
*/
void QSystemTrayIcon::setIcon(const QIcon &icon)
{
    Q_D(QSystemTrayIcon);
    d->icon = icon;
    d->updateIcon_sys();
}

QIcon QSystemTrayIcon::icon() const
{
    Q_D(const QSystemTrayIcon);
    return d->icon;
}

/*!
    \property QSystemTrayIcon::toolTip
    \brief the tooltip for the system tray entry

    On some systems, the tooltip's length is limited. The tooltip will be truncated
    if necessary.
*/
void QSystemTrayIcon::setToolTip(const QString &tooltip)
{
    Q_D(QSystemTrayIcon);
    d->toolTip = tooltip;
    d->updateToolTip_sys();
}

QString QSystemTrayIcon::toolTip() const
{
    Q_D(const QSystemTrayIcon);
    return d->toolTip;
}

/*!
    \fn void QSystemTrayIcon::show()

    Shows the icon in the system tray.

    \sa hide(), visible
*/

/*!
    \fn void QSystemTrayIcon::hide()

    Hides the system tray entry.

    \sa show(), visible
*/

/*!
    \since 4.3
    Returns the geometry of the system tray icon in screen coordinates.

    \sa visible
*/
QRect QSystemTrayIcon::geometry() const
{
    Q_D(const QSystemTrayIcon);
    if (!d->visible)
        return QRect();
    return d->geometry_sys();
}

/*!
    \property QSystemTrayIcon::visible
    \brief whether the system tray entry is visible

    Setting this property to true or calling show() makes the system tray icon
    visible; setting this property to false or calling hide() hides it.
*/
void QSystemTrayIcon::setVisible(bool visible)
{
    Q_D(QSystemTrayIcon);
    if (visible == d->visible)
        return;
    if (Q_UNLIKELY(visible && d->icon.isNull()))
        qWarning("QSystemTrayIcon::setVisible: No Icon set");
    d->visible = visible;
    if (d->visible)
        d->install_sys();
    else
        d->remove_sys();
}

bool QSystemTrayIcon::isVisible() const
{
    Q_D(const QSystemTrayIcon);
    return d->visible;
}

/*!
  \reimp
*/
bool QSystemTrayIcon::event(QEvent *e)
{
    return QObject::event(e);
}

/*!
    \enum QSystemTrayIcon::ActivationReason

     This enum describes the reason the system tray was activated.

     \value Unknown     Unknown reason
     \value Context     The context menu for the system tray entry was requested
     \value DoubleClick The system tray entry was double clicked. \note On macOS, a
        double click will only be emitted if no context menu is set, since the menu
        opens on mouse press
     \value Trigger     The system tray entry was clicked
     \value MiddleClick The system tray entry was clicked with the middle mouse button

     \sa activated()
*/

/*!
    \fn void QSystemTrayIcon::activated(QSystemTrayIcon::ActivationReason reason)

    This signal is emitted when the user activates the system tray icon. \a reason
    specifies the reason for activation. QSystemTrayIcon::ActivationReason enumerates
    the various reasons.

    \sa QSystemTrayIcon::ActivationReason
*/

/*!
    \fn void QSystemTrayIcon::messageClicked()

    This signal is emitted when the message displayed using showMessage()
    was clicked by the user.

    Currently this signal is not sent on \macos.

    \note We follow Microsoft Windows behavior, so the
    signal is also emitted when the user clicks on a tray icon with
    a balloon message displayed.

    \sa activated()
*/


/*!
    Returns \c true if the system tray is available; otherwise returns \c false.

    If the system tray is currently unavailable but becomes available later,
    QSystemTrayIcon will automatically add an entry in the system tray if it
    is \l visible.
*/

bool QSystemTrayIcon::isSystemTrayAvailable()
{
    return QSystemTrayIconPrivate::isSystemTrayAvailable_sys();
}

/*!
    Returns \c true if the system tray supports balloon messages; otherwise returns \c false.

    \sa showMessage()
*/
bool QSystemTrayIcon::supportsMessages()
{
    return QSystemTrayIconPrivate::supportsMessages_sys();
}

/*!
    \fn void QSystemTrayIcon::showMessage(const QString &title, const QString &message, MessageIcon icon, int millisecondsTimeoutHint)
    \since 4.3

    Shows a balloon message for the entry with the given \a title, \a message and
    \a icon for the time specified in \a millisecondsTimeoutHint. \a title and \a message
    must be plain text strings.

    Message can be clicked by the user; the messageClicked() signal will emitted when
    this occurs.

    Note that display of messages are dependent on the system configuration and user
    preferences, and that messages may not appear at all. Hence, it should not be
    relied upon as the sole means for providing critical information.

    On Windows, the \a millisecondsTimeoutHint is usually ignored by the system
    when the application has focus.

    Has been turned into a slot in Qt 5.2.

    \sa show(), supportsMessages()
  */
void QSystemTrayIcon::showMessage(const QString& title, const QString& msg,
                            QSystemTrayIcon::MessageIcon msgIcon, int msecs)
{
    Q_D(QSystemTrayIcon);
    if (d->visible)
        d->showMessage_sys(title, msg, messageIcon2qIcon(msgIcon), msgIcon, msecs);
}

/*!
    \fn void QSystemTrayIcon::showMessage(const QString &title, const QString &message, const QIcon &icon, int millisecondsTimeoutHint)

    \overload showMessage()

    Shows a balloon message for the entry with the given \a title, \a message,
    and custom icon \a icon for the time specified in \a millisecondsTimeoutHint.

    \since 5.9
*/
void QSystemTrayIcon::showMessage(const QString &title, const QString &msg,
                            const QIcon &icon, int msecs)
{
    Q_D(QSystemTrayIcon);
    if (d->visible)
        d->showMessage_sys(title, msg, icon, QSystemTrayIcon::NoIcon, msecs);
}

void QSystemTrayIconPrivate::_q_emitActivated(QPlatformSystemTrayIcon::ActivationReason reason)
{
    Q_Q(QSystemTrayIcon);
    emit q->activated(static_cast<QSystemTrayIcon::ActivationReason>(reason));
}

//////////////////////////////////////////////////////////////////////
static QBalloonTip *theSolitaryBalloonTip = 0;

void QBalloonTip::showBalloon(const QIcon &icon, const QString &title,
                              const QString &message, QSystemTrayIcon *trayIcon,
                              const QPoint &pos, int timeout, bool showArrow)
{
    hideBalloon();
    if (message.isEmpty() && title.isEmpty())
        return;

    theSolitaryBalloonTip = new QBalloonTip(icon, title, message, trayIcon);
    if (timeout < 0)
        timeout = 10000; //10 s default
    theSolitaryBalloonTip->balloon(pos, timeout, showArrow);
}

void QBalloonTip::hideBalloon()
{
    if (!theSolitaryBalloonTip)
        return;
    theSolitaryBalloonTip->hide();
    delete theSolitaryBalloonTip;
    theSolitaryBalloonTip = 0;
}

void QBalloonTip::updateBalloonPosition(const QPoint& pos)
{
    if (!theSolitaryBalloonTip)
        return;
    theSolitaryBalloonTip->hide();
    theSolitaryBalloonTip->balloon(pos, 0, theSolitaryBalloonTip->showArrow);
}

bool QBalloonTip::isBalloonVisible()
{
    return theSolitaryBalloonTip;
}

QBalloonTip::QBalloonTip(const QIcon &icon, const QString &title,
                         const QString &message, QSystemTrayIcon *ti)
    : QWidget(0, Qt::ToolTip),
      trayIcon(ti),
      timerId(-1),
      showArrow(true)
{
    setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(ti, SIGNAL(destroyed()), this, SLOT(close()));

#if QT_CONFIG(label)
    QLabel *titleLabel = new QLabel;
    titleLabel->installEventFilter(this);
    titleLabel->setText(title);
    QFont f = titleLabel->font();
    f.setBold(true);
    titleLabel->setFont(f);
    titleLabel->setTextFormat(Qt::PlainText); // to maintain compat with windows
#endif

    const int iconSize = 18;
    const int closeButtonSize = 15;

#if QT_CONFIG(pushbutton)
    QPushButton *closeButton = new QPushButton;
    closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    closeButton->setIconSize(QSize(closeButtonSize, closeButtonSize));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeButton->setFixedSize(closeButtonSize, closeButtonSize);
    QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
#endif

#if QT_CONFIG(label)
    QLabel *msgLabel = new QLabel;
    msgLabel->installEventFilter(this);
    msgLabel->setText(message);
    msgLabel->setTextFormat(Qt::PlainText);
    msgLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // smart size for the message label
    int limit = QDesktopWidgetPrivate::availableGeometry(msgLabel).size().width() / 3;
    if (msgLabel->sizeHint().width() > limit) {
        msgLabel->setWordWrap(true);
        if (msgLabel->sizeHint().width() > limit) {
            msgLabel->d_func()->ensureTextControl();
            if (QWidgetTextControl *control = msgLabel->d_func()->control) {
                QTextOption opt = control->document()->defaultTextOption();
                opt.setWrapMode(QTextOption::WrapAnywhere);
                control->document()->setDefaultTextOption(opt);
            }
        }
        // Here we allow the text being much smaller than the balloon widget
        // to emulate the weird standard windows behavior.
        msgLabel->setFixedSize(limit, msgLabel->heightForWidth(limit));
    }
#endif

    QGridLayout *layout = new QGridLayout;
#if QT_CONFIG(label)
    if (!icon.isNull()) {
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
        iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        iconLabel->setMargin(2);
        layout->addWidget(iconLabel, 0, 0);
        layout->addWidget(titleLabel, 0, 1);
    } else {
        layout->addWidget(titleLabel, 0, 0, 1, 2);
    }
#endif

#if QT_CONFIG(pushbutton)
    layout->addWidget(closeButton, 0, 2);
#endif

#if QT_CONFIG(label)
    layout->addWidget(msgLabel, 1, 0, 1, 3);
#endif
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setMargin(3);
    setLayout(layout);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0xff, 0xff, 0xe1));
    pal.setColor(QPalette::WindowText, Qt::black);
    setPalette(pal);
}

QBalloonTip::~QBalloonTip()
{
    theSolitaryBalloonTip = 0;
}

void QBalloonTip::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), pixmap);
}

void QBalloonTip::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
}

void QBalloonTip::balloon(const QPoint& pos, int msecs, bool showArrow)
{
    this->showArrow = showArrow;
    QRect scr = QDesktopWidgetPrivate::screenGeometry(pos);
    QSize sh = sizeHint();
    const int border = 1;
    const int ah = 18, ao = 18, aw = 18, rc = 7;
    bool arrowAtTop = (pos.y() + sh.height() + ah < scr.height());
    bool arrowAtLeft = (pos.x() + sh.width() - ao < scr.width());
    setContentsMargins(border + 3,  border + (arrowAtTop ? ah : 0) + 2, border + 3, border + (arrowAtTop ? 0 : ah) + 2);
    updateGeometry();
    sh  = sizeHint();

    int ml, mr, mt, mb;
    QSize sz = sizeHint();
    if (!arrowAtTop) {
        ml = mt = 0;
        mr = sz.width() - 1;
        mb = sz.height() - ah - 1;
    } else {
        ml = 0;
        mt = ah;
        mr = sz.width() - 1;
        mb = sz.height() - 1;
    }

    QPainterPath path;
#if defined(QT_NO_XSHAPE) && 0 /* Used to be included in Qt4 for Q_WS_X11 */
    // XShape is required for setting the mask, so we just
    // draw an ugly square when its not available
    path.moveTo(0, 0);
    path.lineTo(sz.width() - 1, 0);
    path.lineTo(sz.width() - 1, sz.height() - 1);
    path.lineTo(0, sz.height() - 1);
    path.lineTo(0, 0);
    move(qMax(pos.x() - sz.width(), scr.left()), pos.y());
#else
    path.moveTo(ml + rc, mt);
    if (arrowAtTop && arrowAtLeft) {
        if (showArrow) {
            path.lineTo(ml + ao, mt);
            path.lineTo(ml + ao, mt - ah);
            path.lineTo(ml + ao + aw, mt);
        }
        move(qMax(pos.x() - ao, scr.left() + 2), pos.y());
    } else if (arrowAtTop && !arrowAtLeft) {
        if (showArrow) {
            path.lineTo(mr - ao - aw, mt);
            path.lineTo(mr - ao, mt - ah);
            path.lineTo(mr - ao, mt);
        }
        move(qMin(pos.x() - sh.width() + ao, scr.right() - sh.width() - 2), pos.y());
    }
    path.lineTo(mr - rc, mt);
    path.arcTo(QRect(mr - rc*2, mt, rc*2, rc*2), 90, -90);
    path.lineTo(mr, mb - rc);
    path.arcTo(QRect(mr - rc*2, mb - rc*2, rc*2, rc*2), 0, -90);
    if (!arrowAtTop && !arrowAtLeft) {
        if (showArrow) {
            path.lineTo(mr - ao, mb);
            path.lineTo(mr - ao, mb + ah);
            path.lineTo(mr - ao - aw, mb);
        }
        move(qMin(pos.x() - sh.width() + ao, scr.right() - sh.width() - 2),
             pos.y() - sh.height());
    } else if (!arrowAtTop && arrowAtLeft) {
        if (showArrow) {
            path.lineTo(ao + aw, mb);
            path.lineTo(ao, mb + ah);
            path.lineTo(ao, mb);
        }
        move(qMax(pos.x() - ao, scr.x() + 2), pos.y() - sh.height());
    }
    path.lineTo(ml + rc, mb);
    path.arcTo(QRect(ml, mb - rc*2, rc*2, rc*2), -90, -90);
    path.lineTo(ml, mt + rc);
    path.arcTo(QRect(ml, mt, rc*2, rc*2), 180, -90);

    // Set the mask
    QBitmap bitmap = QBitmap(sizeHint());
    bitmap.fill(Qt::color0);
    QPainter painter1(&bitmap);
    painter1.setPen(QPen(Qt::color1, border));
    painter1.setBrush(QBrush(Qt::color1));
    painter1.drawPath(path);
    setMask(bitmap);
#endif

    // Draw the border
    pixmap = QPixmap(sz);
    QPainter painter2(&pixmap);
    painter2.setPen(QPen(palette().color(QPalette::Window).darker(160), border));
    painter2.setBrush(palette().color(QPalette::Window));
    painter2.drawPath(path);

    if (msecs > 0)
        timerId = startTimer(msecs);
    show();
}

void QBalloonTip::mousePressEvent(QMouseEvent *e)
{
    close();
    if(e->button() == Qt::LeftButton)
        emit trayIcon->messageClicked();
}

void QBalloonTip::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timerId) {
        killTimer(timerId);
        if (!underMouse())
            close();
        return;
    }
    QWidget::timerEvent(e);
}

//////////////////////////////////////////////////////////////////////
void QSystemTrayIconPrivate::install_sys_qpa()
{
    qpa_sys->init();
    QObject::connect(qpa_sys, SIGNAL(activated(QPlatformSystemTrayIcon::ActivationReason)),
                     q_func(), SLOT(_q_emitActivated(QPlatformSystemTrayIcon::ActivationReason)));
    QObject::connect(qpa_sys, &QPlatformSystemTrayIcon::messageClicked,
                     q_func(), &QSystemTrayIcon::messageClicked);
    updateMenu_sys();
    updateIcon_sys();
    updateToolTip_sys();
}

void QSystemTrayIconPrivate::remove_sys_qpa()
{
    QObject::disconnect(qpa_sys, SIGNAL(activated(QPlatformSystemTrayIcon::ActivationReason)),
                        q_func(), SLOT(_q_emitActivated(QPlatformSystemTrayIcon::ActivationReason)));
    QObject::disconnect(qpa_sys, &QPlatformSystemTrayIcon::messageClicked,
                        q_func(), &QSystemTrayIcon::messageClicked);
    qpa_sys->cleanup();
}

void QSystemTrayIconPrivate::addPlatformMenu(QMenu *menu) const
{
#if QT_CONFIG(menu)
    if (menu->platformMenu())
        return; // The platform menu already exists.

    // The recursion depth is the same as menu depth, so should not
    // be higher than 3 levels.
    const auto actions = menu->actions();
    for (QAction *action : actions) {
        if (action->menu())
            addPlatformMenu(action->menu());
    }

    // This menu should be processed *after* its children, otherwise
    // setMenu() is not called on respective QPlatformMenuItems.
    QPlatformMenu *platformMenu = qpa_sys->createMenu();
    if (platformMenu)
        menu->setPlatformMenu(platformMenu);
#else
    Q_UNUSED(menu)
#endif // QT_CONFIG(menu)
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON

#include "moc_qsystemtrayicon.cpp"
#include "moc_qsystemtrayicon_p.cpp"
