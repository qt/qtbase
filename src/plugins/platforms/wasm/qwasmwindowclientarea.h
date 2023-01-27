// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOWCLIENTAREA_H
#define QWASMWINDOWCLIENTAREA_H

#include <QtCore/qnamespace.h>
#include <qpa/qwindowsysteminterface.h>

#include <emscripten/val.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace qstdweb {
class EventCallback;
}

struct PointerEvent;
class QWasmScreen;
class QWasmWindow;

class ClientArea
{
public:
    ClientArea(QWasmWindow *window, QWasmScreen *screen, emscripten::val element);

private:
    bool processPointer(const PointerEvent &event);
    bool deliverEvent(const PointerEvent &event);

    std::unique_ptr<qstdweb::EventCallback> m_pointerDownCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerMoveCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerUpCallback;
    std::unique_ptr<qstdweb::EventCallback> m_pointerCancelCallback;

    QMap<int, QWindowSystemInterface::TouchPoint> m_pointerIdToTouchPoints;

    QWasmScreen *m_screen;
    QWasmWindow *m_window;
    emscripten::val m_element;
};

QT_END_NAMESPACE
#endif // QWASMWINDOWNONCLIENTAREA_H
