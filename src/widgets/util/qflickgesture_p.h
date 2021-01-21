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

#ifndef QFLICKGESTURE_P_H
#define QFLICKGESTURE_P_H

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
#include "qevent.h"
#include "qgesturerecognizer.h"
#include "private/qgesture_p.h"
#include "qscroller.h"
#include "qscopedpointer.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

class QFlickGesturePrivate;
class QGraphicsItem;

class Q_WIDGETS_EXPORT QFlickGesture : public QGesture
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFlickGesture)

public:
    QFlickGesture(QObject *receiver, Qt::MouseButton button, QObject *parent = nullptr);
    ~QFlickGesture();

    friend class QFlickGestureRecognizer;
};

class QFlickGesturePrivate : public QGesturePrivate
{
    Q_DECLARE_PUBLIC(QFlickGesture)
public:
    QFlickGesturePrivate();

    QPointer<QObject> receiver;
    QScroller *receiverScroller;
    Qt::MouseButton button; // NoButton == Touch
    bool macIgnoreWheel;
};

class QFlickGestureRecognizer : public QGestureRecognizer
{
public:
    QFlickGestureRecognizer(Qt::MouseButton button);

    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
    void reset(QGesture *state) override;

private:
    Qt::MouseButton button; // NoButton == Touch
};

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QFLICKGESTURE_P_H
