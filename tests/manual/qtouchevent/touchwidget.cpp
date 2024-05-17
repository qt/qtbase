// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
