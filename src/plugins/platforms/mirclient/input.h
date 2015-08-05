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

#ifndef UBUNTU_INPUT_H
#define UBUNTU_INPUT_H

// Qt
#include <qpa/qwindowsysteminterface.h>

#include <mir_toolkit/mir_client_library.h>

class UbuntuClientIntegration;
class UbuntuWindow;

class UbuntuInput : public QObject
{
    Q_OBJECT

public:
    UbuntuInput(UbuntuClientIntegration* integration);
    virtual ~UbuntuInput();

    // QObject methods.
    void customEvent(QEvent* event) override;

    void postEvent(UbuntuWindow* window, const MirEvent *event);
    UbuntuClientIntegration* integration() const { return mIntegration; }

protected:
    void dispatchKeyEvent(QWindow *window, const MirInputEvent *event);
    void dispatchPointerEvent(QWindow *window, const MirInputEvent *event);
    void dispatchTouchEvent(QWindow *window, const MirInputEvent *event);
    void dispatchInputEvent(QWindow *window, const MirInputEvent *event);
    
    void dispatchOrientationEvent(QWindow* window, const MirOrientationEvent *event);

private:
    UbuntuClientIntegration* mIntegration;
    QTouchDevice* mTouchDevice;
    const QByteArray mEventFilterType;
    const QEvent::Type mEventType;
};

#endif // UBUNTU_INPUT_H
