/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QEVENT_P_H
#define QEVENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of other Qt classes. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

class QPointingDevice;

class Q_GUI_EXPORT QMutableTouchEvent : public QTouchEvent
{
public:
    QMutableTouchEvent(QEvent::Type eventType = QEvent::TouchBegin,
                       const QPointingDevice *device = nullptr,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                       const QList<QEventPoint> &touchPoints = QList<QEventPoint>()) :
        QTouchEvent(eventType, device, modifiers, touchPoints) { }
    ~QMutableTouchEvent() override;

    static QMutableTouchEvent *from(QTouchEvent *e) { return static_cast<QMutableTouchEvent *>(e); }

    static QMutableTouchEvent &from(QTouchEvent &e) { return static_cast<QMutableTouchEvent &>(e); }

    void setTarget(QObject *target) { m_target = target; }

    void addPoint(const QEventPoint &point);
};

class Q_GUI_EXPORT QMutableSinglePointEvent : public QSinglePointEvent
{
public:
    QMutableSinglePointEvent(const QSinglePointEvent &other) : QSinglePointEvent(other) {}
    QMutableSinglePointEvent(Type type = QEvent::None, const QPointingDevice *device = nullptr, const QEventPoint &point = QEventPoint(),
                             Qt::MouseButton button = Qt::NoButton, Qt::MouseButtons buttons = Qt::NoButton,
                             Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                             Qt::MouseEventSource source = Qt::MouseEventSynthesizedByQt) :
        QSinglePointEvent(type, device, point, button, buttons, modifiers, source) { }
    ~QMutableSinglePointEvent() override;

    static QMutableSinglePointEvent *from(QSinglePointEvent *e) { return static_cast<QMutableSinglePointEvent *>(e); }

    static QMutableSinglePointEvent &from(QSinglePointEvent &e) { return static_cast<QMutableSinglePointEvent &>(e); }

    QMutableEventPoint &mutablePoint() { return QMutableEventPoint::from(point(0)); }

    void setSource(Qt::MouseEventSource s) { m_source = s; }

    bool isDoubleClick() { return m_doubleClick; }

    void setDoubleClick(bool d = true) { m_doubleClick = d; }
};

QT_END_NAMESPACE

#endif // QEVENT_P_H
