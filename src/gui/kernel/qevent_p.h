// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    void setTarget(QObject *target) { m_target = target; }
    void addPoint(const QEventPoint &point);

    static void setTarget(QTouchEvent *e, QObject *target) { e->m_target = target; }
    static void addPoint(QTouchEvent *e, const QEventPoint &point);
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

    void setSource(Qt::MouseEventSource s) { m_source = s; }

    bool isDoubleClick() { return m_doubleClick; }

    void setDoubleClick(bool d = true) { m_doubleClick = d; }

    static bool isDoubleClick(const QSinglePointEvent *ev)
    {
        return ev->m_doubleClick;
    }

    static void setDoubleClick(QSinglePointEvent *ev, bool d)
    {
        ev->m_doubleClick = d;
    }
};

QT_END_NAMESPACE

#endif // QEVENT_P_H
