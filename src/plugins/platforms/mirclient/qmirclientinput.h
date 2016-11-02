/****************************************************************************
**
** Copyright (C) 2014-2016 Canonical, Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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


#ifndef QMIRCLIENTINPUT_H
#define QMIRCLIENTINPUT_H

// Qt
#include <qpa/qwindowsysteminterface.h>

#include <mir_toolkit/mir_client_library.h>

class QMirClientClientIntegration;
class QMirClientWindow;

class QMirClientInput : public QObject
{
    Q_OBJECT

public:
    QMirClientInput(QMirClientClientIntegration* integration);
    virtual ~QMirClientInput();

    // QObject methods.
    void customEvent(QEvent* event) override;

    void postEvent(QMirClientWindow* window, const MirEvent *event);
    QMirClientClientIntegration* integration() const { return mIntegration; }
    QMirClientWindow *lastInputWindow() const {return mLastInputWindow; }

protected:
    void dispatchKeyEvent(QMirClientWindow *window, const MirInputEvent *event);
    void dispatchPointerEvent(QMirClientWindow *window, const MirInputEvent *event);
    void dispatchTouchEvent(QMirClientWindow *window, const MirInputEvent *event);
    void dispatchInputEvent(QMirClientWindow *window, const MirInputEvent *event);

    void dispatchOrientationEvent(QWindow* window, const MirOrientationEvent *event);
    void handleSurfaceEvent(const QPointer<QMirClientWindow> &window, const MirSurfaceEvent *event);
    void handleSurfaceOutputEvent(const QPointer<QMirClientWindow> &window, const MirSurfaceOutputEvent *event);

private:
    QMirClientClientIntegration* mIntegration;
    QTouchDevice* mTouchDevice;
    const QByteArray mEventFilterType;
    const QEvent::Type mEventType;

    QMirClientWindow *mLastInputWindow;
};

#endif // QMIRCLIENTINPUT_H
