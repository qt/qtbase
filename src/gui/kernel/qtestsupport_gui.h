/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QTESTSUPPORT_GUI_H
#define QTESTSUPPORT_GUI_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qevent.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

class QWindow;

Q_GUI_EXPORT  void qt_handleTouchEvent(QWindow *w, const QPointingDevice *device,
                                const QList<QEventPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier);

namespace QTest {

[[nodiscard]] Q_GUI_EXPORT bool qWaitForWindowActive(QWindow *window, int timeout = 5000);
[[nodiscard]] Q_GUI_EXPORT bool qWaitForWindowExposed(QWindow *window, int timeout = 5000);

Q_GUI_EXPORT QPointingDevice * createTouchDevice(QInputDevice::DeviceType devType = QInputDevice::DeviceType::TouchScreen,
                                                 QInputDevice::Capabilities caps = QInputDevice::Capability::Position);

class Q_GUI_EXPORT QTouchEventSequence
{
public:
    virtual ~QTouchEventSequence();
    QTouchEventSequence& press(int touchId, const QPoint &pt, QWindow *window = nullptr);
    QTouchEventSequence& move(int touchId, const QPoint &pt, QWindow *window = nullptr);
    QTouchEventSequence& release(int touchId, const QPoint &pt, QWindow *window = nullptr);
    virtual QTouchEventSequence& stationary(int touchId);

    virtual void commit(bool processEvents = true);

protected:
    QTouchEventSequence(QWindow *window, QPointingDevice *aDevice, bool autoCommit);

    QPoint mapToScreen(QWindow *window, const QPoint &pt);

    QEventPoint &point(int touchId);

    QEventPoint &pointOrPreviousPoint(int touchId);

    QMap<int, QEventPoint> previousPoints;
    QMap<int, QEventPoint> points;
    QWindow *targetWindow;
    QPointingDevice *device;
    bool commitWhenDestroyed;
    friend QTouchEventSequence touchEvent(QWindow *window, QPointingDevice *device, bool autoCommit);
};

} // namespace QTest

QT_END_NAMESPACE

#endif
