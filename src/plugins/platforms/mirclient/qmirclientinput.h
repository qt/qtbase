/*
 * Copyright (C) 2014-2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

protected:
    void dispatchKeyEvent(QWindow *window, const MirInputEvent *event);
    void dispatchPointerEvent(QWindow *window, const MirInputEvent *event);
    void dispatchTouchEvent(QWindow *window, const MirInputEvent *event);
    void dispatchInputEvent(QWindow *window, const MirInputEvent *event);
    
    void dispatchOrientationEvent(QWindow* window, const MirOrientationEvent *event);

private:
    QMirClientClientIntegration* mIntegration;
    QTouchDevice* mTouchDevice;
    const QByteArray mEventFilterType;
    const QEvent::Type mEventType;
};

#endif // QMIRCLIENTINPUT_H
