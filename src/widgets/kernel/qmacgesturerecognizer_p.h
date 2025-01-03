// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACSWIPEGESTURERECOGNIZER_MAC_P_H
#define QMACSWIPEGESTURERECOGNIZER_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qtimer.h"
#include "qpoint.h"
#include "qgesturerecognizer.h"
#include <QtCore/qpointer.h>

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

class QMacSwipeGestureRecognizer : public QGestureRecognizer
{
public:
    QMacSwipeGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *gesture, QObject *watched, QEvent *event) override;
    void reset(QGesture *gesture) override;
};

class QMacPinchGestureRecognizer : public QGestureRecognizer
{
public:
    QMacPinchGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *gesture, QObject *watched, QEvent *event) override;
    void reset(QGesture *gesture) override;
};

class QMacPanGestureRecognizer : public QObject, public QGestureRecognizer
{
public:
    QMacPanGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *gesture, QObject *watched, QEvent *event) override;
    void reset(QGesture *gesture) override;
protected:
    void timerEvent(QTimerEvent *ev) override;
private:
    QPointF _startPos;
    QBasicTimer _panTimer;
    bool _panCanceled;
    QPointer<QObject> _target;
};

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QMACSWIPEGESTURERECOGNIZER_MAC_P_H
