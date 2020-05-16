/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "qdesktopwidget_p.h"
#include "qscreen.h"
#include "qwidget_p.h"
#include "qwindow.h"

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

QDesktopScreenWidget::QDesktopScreenWidget(QScreen *screen, const QRect &geometry)
    : QWidget(nullptr, Qt::Desktop)
{
    setVisible(false);
    if (QWindow *winHandle = windowHandle())
        winHandle->setScreen(screen);
    setGeometry(geometry);
}

QScreen *QDesktopScreenWidget::screen() const
{
    const QDesktopWidgetPrivate *desktopWidgetP
        = static_cast<const QDesktopWidgetPrivate *>(qt_widget_private(QApplication::desktop()));
    for (auto it : qAsConst(desktopWidgetP->screenWidgets)) {
        if (it.second == this)
            return it.first;
    }
    return nullptr;
}

QDesktopWidgetPrivate::~QDesktopWidgetPrivate()
{
    qDeleteAll(screenWidgets.values());
}

void QDesktopWidgetPrivate::updateScreens()
{
    Q_Q(QDesktopWidget);
    const QList<QScreen *> screenList = QGuiApplication::screens();

    // Re-build our screens list. This is the easiest way to later compute which signals to emit.
    // Create new screen widgets as necessary.
    // Furthermore, we note which screens have changed, and compute the overall virtual geometry.
    QFlatMap<QScreen*, QDesktopScreenWidget*> newScreenWidgets;
    QRegion virtualGeometry;

    for (QScreen *screen : screenList) {
        const QRect screenGeometry = screen->geometry();
        QDesktopScreenWidget *screenWidget = screenWidgets.value(screen);
        if (!screenWidget) {
            // a new screen, create a widget and connect the signals.
            screenWidget = new QDesktopScreenWidget(screen, screenGeometry);
            QObjectPrivate::connect(screen, &QScreen::geometryChanged,
                                    this, &QDesktopWidgetPrivate::updateScreens, Qt::QueuedConnection);
            QObjectPrivate::connect(screen, &QObject::destroyed,
                                    this, &QDesktopWidgetPrivate::updateScreens, Qt::QueuedConnection);
        }
        // record all the screens and the overall geometry.
        newScreenWidgets.insert(screen, screenWidget);
        virtualGeometry += screenGeometry;
    }

    // Now we apply the accumulated updates.
    qSwap(screenWidgets, newScreenWidgets); // now [newScreenWidgets] is the old screen list
    Q_ASSERT(screenWidgets.size() == screenList.length());
    q->setGeometry(virtualGeometry.boundingRect());

    // Delete the QDesktopScreenWidget that are not used any more.
    for (auto it : qAsConst(newScreenWidgets)) {
        if (!screenWidgets.contains(it.first))
            delete it.second;
    }
}

QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, nullptr, Qt::Desktop)
{
    Q_D(QDesktopWidget);
    setObjectName(QLatin1String("desktop"));
    d->updateScreens();
    QObjectPrivate::connect(qApp, &QApplication::screenAdded, d, &QDesktopWidgetPrivate::updateScreens);
}

QDesktopWidget::~QDesktopWidget() = default;

QT_END_NAMESPACE

#include "moc_qdesktopwidget_p.cpp"
