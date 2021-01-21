/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
