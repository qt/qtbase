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

#include "qglobal.h"
#include "qdesktopwidget.h"
#include "qdesktopwidget_p.h"
#include "qscreen.h"
#include "qwidget_p.h"
#include "qwindow.h"

QT_BEGIN_NAMESPACE

int QDesktopScreenWidget::screenNumber() const
{
    const QDesktopWidgetPrivate *desktopWidgetP
        = static_cast<const QDesktopWidgetPrivate *>(qt_widget_private(QApplication::desktop()));
    return desktopWidgetP->screens.indexOf(const_cast<QDesktopScreenWidget *>(this));
}

const QRect QDesktopWidget::screenGeometry(const QWidget *widget) const
{
    if (Q_UNLIKELY(!widget)) {
        qWarning("QDesktopWidget::screenGeometry(): Attempt "
                 "to get the screen geometry of a null widget");
        return QRect();
    }
    QRect rect = QWidgetPrivate::screenGeometry(widget);
    if (rect.isNull())
        return screenGeometry(screenNumber(widget));
    else return rect;
}

const QRect QDesktopWidget::availableGeometry(const QWidget *widget) const
{
    if (Q_UNLIKELY(!widget)) {
        qWarning("QDesktopWidget::availableGeometry(): Attempt "
                 "to get the available geometry of a null widget");
        return QRect();
    }
    QRect rect = QWidgetPrivate::screenGeometry(widget);
    if (rect.isNull())
        return availableGeometry(screenNumber(widget));
    else
        return rect;
}

void QDesktopWidgetPrivate::_q_updateScreens()
{
    Q_Q(QDesktopWidget);
    const QList<QScreen *> screenList = QGuiApplication::screens();
    const int targetLength = screenList.length();
    const int oldLength = screens.length();

    // Add or remove screen widgets as necessary
    while (screens.size() > targetLength)
        delete screens.takeLast();

    for (int currentLength = screens.size(); currentLength < targetLength; ++currentLength) {
        QScreen *qScreen = screenList.at(currentLength);
        QDesktopScreenWidget *screenWidget = new QDesktopScreenWidget;
        screenWidget->setGeometry(qScreen->geometry());
        QObject::connect(qScreen, SIGNAL(geometryChanged(QRect)),
                         q, SLOT(_q_updateScreens()), Qt::QueuedConnection);
        QObject::connect(qScreen, SIGNAL(availableGeometryChanged(QRect)),
                         q, SLOT(_q_availableGeometryChanged()), Qt::QueuedConnection);
        QObject::connect(qScreen, SIGNAL(destroyed()),
                         q, SLOT(_q_updateScreens()), Qt::QueuedConnection);
        screens.append(screenWidget);
    }

    QRegion virtualGeometry;

    // update the geometry of each screen widget, determine virtual geometry,
    // set the new screen for window handle and emit change signals afterwards.
    QList<int> changedScreens;
    for (int i = 0; i < screens.length(); i++) {
        QDesktopScreenWidget *screenWidget = screens.at(i);
        QScreen *qScreen = screenList.at(i);
        QWindow *winHandle = screenWidget->windowHandle();
        if (winHandle && winHandle->screen() != qScreen)
            winHandle->setScreen(qScreen);
        const QRect screenGeometry = qScreen->geometry();
        if (screenGeometry != screenWidget->geometry()) {
            screenWidget->setGeometry(screenGeometry);
            changedScreens.push_back(i);
        }
        virtualGeometry += screenGeometry;
    }

    q->setGeometry(virtualGeometry.boundingRect());

    if (oldLength != targetLength)
        emit q->screenCountChanged(targetLength);

    foreach (int changedScreen, changedScreens)
        emit q->resized(changedScreen);
}

void QDesktopWidgetPrivate::_q_availableGeometryChanged()
{
    Q_Q(QDesktopWidget);
    if (QScreen *screen = qobject_cast<QScreen *>(q->sender()))
        emit q->workAreaResized(QGuiApplication::screens().indexOf(screen));
}

QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    Q_D(QDesktopWidget);
    setObjectName(QLatin1String("desktop"));
    d->_q_updateScreens();
    connect(qApp, SIGNAL(screenAdded(QScreen*)), this, SLOT(_q_updateScreens()));
    connect(qApp, SIGNAL(primaryScreenChanged(QScreen*)), this, SIGNAL(primaryScreenChanged()));
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return QGuiApplication::primaryScreen()->virtualSiblings().size() > 1;
}

int QDesktopWidget::primaryScreen() const
{
    return 0;
}

int QDesktopWidget::numScreens() const
{
    return qMax(QGuiApplication::screens().size(), 1);
}

QWidget *QDesktopWidget::screen(int screen)
{
    Q_D(QDesktopWidget);
    if (screen < 0 || screen >= d->screens.length())
        return d->screens.at(0);
    return d->screens.at(screen);
}

const QRect QDesktopWidget::availableGeometry(int screenNo) const
{
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screenNo == -1)
        screenNo = 0;
    if (screenNo < 0 || screenNo >= screens.size())
        return QRect();
    else
        return screens.at(screenNo)->availableGeometry();
}

const QRect QDesktopWidget::screenGeometry(int screenNo) const
{
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screenNo == -1)
        screenNo = 0;
    if (screenNo < 0 || screenNo >= screens.size())
        return QRect();
    else
        return screens.at(screenNo)->geometry();
}

int QDesktopWidget::screenNumber(const QWidget *w) const
{
    if (!w)
        return primaryScreen();

    const QList<QScreen *> allScreens = QGuiApplication::screens();
    QList<QScreen *> screens = allScreens;
    if (screens.isEmpty()) // This should never happen
        return primaryScreen();

    // If there is more than one virtual desktop
    if (screens.count() != screens.constFirst()->virtualSiblings().count()) {
        // Find the root widget, get a QScreen from it and use the
        // virtual siblings for checking the window position.
        const QWidget *root = w;
        const QWidget *tmp = w;
        while ((tmp = tmp->parentWidget()))
            root = tmp;
        const QWindow *winHandle = root->windowHandle();
        if (winHandle) {
            const QScreen *winScreen = winHandle->screen();
            if (winScreen)
                screens = winScreen->virtualSiblings();
        }
    }

    // Get the screen number from window position using screen geometry
    // and proper screens.
    QRect frame = w->frameGeometry();
    if (!w->isWindow())
        frame.moveTopLeft(w->mapToGlobal(QPoint(0, 0)));

    QScreen *widgetScreen = Q_NULLPTR;
    int largestArea = 0;
    foreach (QScreen *screen, screens) {
        QRect intersected = screen->geometry().intersected(frame);
        int area = intersected.width() * intersected.height();
        if (largestArea < area) {
            widgetScreen = screen;
            largestArea = area;
        }
    }
    return allScreens.indexOf(widgetScreen);
}

int QDesktopWidget::screenNumber(const QPoint &p) const
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (!screens.isEmpty()) {
        const QList<QScreen *> primaryScreens = screens.first()->virtualSiblings();
        // Find the screen index on the primary virtual desktop first
        foreach (QScreen *screen, primaryScreens) {
            if (screen->geometry().contains(p))
                return screens.indexOf(screen);
        }
        // If the screen index is not found on primary virtual desktop, find
        // the screen index on all screens except the first which was for
        // sure in the previous loop. Some other screens may repeat. Find
        // only when there is more than one virtual desktop.
        if (screens.count() != primaryScreens.count()) {
            for (int i = 1; i < screens.size(); ++i) {
                if (screens[i]->geometry().contains(p))
                    return i;
            }
        }
    }
    return primaryScreen(); //even better would be closest screen
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
}

QT_END_NAMESPACE

#include "moc_qdesktopwidget.cpp"
#include "moc_qdesktopwidget_p.cpp"
