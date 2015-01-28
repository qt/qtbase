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

#include "touchwidget.h"

#include <QApplication>
#include <QtEvents>
#include <QTimer>
#include <QTouchEvent>

void TouchWidget::reset()
{
    acceptTouchBegin
        = acceptTouchUpdate
        = acceptTouchEnd
        = seenTouchBegin
        = seenTouchUpdate
        = seenTouchEnd
        = closeWindowOnTouchEnd

        = acceptMousePress
        = acceptMouseMove
        = acceptMouseRelease
        = seenMousePress
        = seenMouseMove
        = seenMouseRelease
        = closeWindowOnMouseRelease

        = false;
    touchPointCount = 0;
}

bool TouchWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
        if (seenTouchBegin) qWarning("TouchBegin: already seen a TouchBegin");
        if (seenTouchUpdate) qWarning("TouchBegin: TouchUpdate cannot happen before TouchBegin");
        if (seenTouchEnd) qWarning("TouchBegin: TouchEnd cannot happen before TouchBegin");
        seenTouchBegin = !seenTouchBegin && !seenTouchUpdate && !seenTouchEnd;
        touchPointCount = qMax(touchPointCount, static_cast<QTouchEvent *>(event)->touchPoints().count());
        if (acceptTouchBegin) {
            event->accept();
            return true;
        }
        break;
    case QEvent::TouchUpdate:
        if (!seenTouchBegin) qWarning("TouchUpdate: have not seen TouchBegin");
        if (seenTouchEnd) qWarning("TouchUpdate: TouchEnd cannot happen before TouchUpdate");
        seenTouchUpdate = seenTouchBegin && !seenTouchEnd;
        touchPointCount = qMax(touchPointCount, static_cast<QTouchEvent *>(event)->touchPoints().count());
        if (acceptTouchUpdate) {
            event->accept();
            return true;
        }
        break;
    case QEvent::TouchEnd:
        if (!seenTouchBegin) qWarning("TouchEnd: have not seen TouchBegin");
        if (seenTouchEnd) qWarning("TouchEnd: already seen a TouchEnd");
        seenTouchEnd = seenTouchBegin && !seenTouchEnd;
        touchPointCount = qMax(touchPointCount, static_cast<QTouchEvent *>(event)->touchPoints().count());
        if (closeWindowOnTouchEnd)
            window()->close();
        if (acceptTouchEnd) {
            event->accept();
            return true;
        }
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        seenMousePress = true;
        if (acceptMousePress) {
            event->accept();
            return true;
        }
        break;
    case QEvent::MouseMove:
        seenMouseMove = true;
        if (acceptMouseMove) {
            event->accept();
            return true;
        }
        break;
    case QEvent::MouseButtonRelease:
        seenMouseRelease = true;
        if (closeWindowOnMouseRelease)
            window()->close();
        if (acceptMouseRelease) {
            event->accept();
            return true;
        }
        break;
    default:
        break;
    }
    return QWidget::event(event);
}
