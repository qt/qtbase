/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include "mousestatwidget.h"

#include <QTabletEvent>
#include <QPainter>
#include <QTextOption>
#include <QTest>

MouseStatWidget::MouseStatWidget(bool acceptTabletEvent):acceptTabletEvent(acceptTabletEvent),
    receivedMouseEventCount(0),
    receivedMouseEventCountToPaint(0),
    receivedTabletEventCount(0),
    receivedTabletEventCountToPaint(0)
{
    startTimer(1000);
}


void MouseStatWidget::tabletEvent(QTabletEvent *event)
{
    ++receivedTabletEventCount;
    if (acceptTabletEvent)
        event->accept();
    else
        event->ignore();
    // make sure the event loop is slow
    QTest::qSleep(15);
}

void MouseStatWidget::mouseMoveEvent(QMouseEvent *)
{
    ++receivedMouseEventCount;
}

void MouseStatWidget::timerEvent(QTimerEvent *)
{
    receivedMouseEventCountToPaint = receivedMouseEventCount;
    receivedTabletEventCountToPaint = receivedTabletEventCount;
    receivedMouseEventCount = 0;
    receivedTabletEventCount = 0;
    update();
}

void MouseStatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.drawRect(rect());
    QStringList text;
    text << ((acceptTabletEvent) ? " - tablet events accepted - " : " - tablet events ignored - ");
    text << QString("Number of tablet events received in the last second: %1").arg(QString::number(receivedTabletEventCountToPaint));
    text << QString("Number of mouse events received in the last second: %1").arg(QString::number(receivedMouseEventCountToPaint));

    QTextOption textOption(Qt::AlignCenter);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    painter.drawText(rect(),
                     text.join("\n"),
                     textOption);
}
