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

#ifndef QSTANDARDGESTURES_P_H
#define QSTANDARDGESTURES_P_H

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
#include "qgesturerecognizer.h"
#include "private/qgesture_p.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

class QPanGestureRecognizer : public QGestureRecognizer
{
public:
    explicit QPanGestureRecognizer(int pointCount = 2) : m_pointCount(pointCount) {}

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;

private:
    const int m_pointCount;
};

class QPinchGestureRecognizer : public QGestureRecognizer
{
public:
    QPinchGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;
};

class QSwipeGestureRecognizer : public QGestureRecognizer
{
public:
    QSwipeGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;
};

class QTapGestureRecognizer : public QGestureRecognizer
{
public:
    QTapGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;
};

class QTapAndHoldGestureRecognizer : public QGestureRecognizer
{
public:
    QTapAndHoldGestureRecognizer();

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;
};

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QSTANDARDGESTURES_P_H
