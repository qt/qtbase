/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

    static QMutableSinglePointEvent *from(QSinglePointEvent *e) { return static_cast<QMutableSinglePointEvent *>(e); }

    static QMutableSinglePointEvent &from(QSinglePointEvent &e) { return static_cast<QMutableSinglePointEvent &>(e); }

    QMutableEventPoint &mutablePoint() { return QMutableEventPoint::from(point(0)); }

    void setSource(Qt::MouseEventSource s) { m_source = s; }

    bool isDoubleClick() { return m_doubleClick; }

    void setDoubleClick(bool d = true) { m_doubleClick = d; }
};

QT_END_NAMESPACE

#endif // QEVENT_P_H
