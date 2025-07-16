// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMSUSPENDRESUMECONTROL_P_H
#define QWASMSUSPENDRESUMECONTROL_P_H

#include <QtCore/qglobal.h>
#include <emscripten/val.h>
#include <map>
#include <functional>
#include <chrono>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

class Q_CORE_EXPORT QWasmSuspendResumeControl
{
public:
    QWasmSuspendResumeControl();
    ~QWasmSuspendResumeControl();

    QWasmSuspendResumeControl(const QWasmSuspendResumeControl&) = delete;
    QWasmSuspendResumeControl& operator=(const QWasmSuspendResumeControl&) = delete;

    static QWasmSuspendResumeControl *get();

    uint32_t registerEventHandler(std::function<void(emscripten::val)> handler);
    void removeEventHandler(uint32_t index);
    emscripten::val jsEventHandlerAt(uint32_t index);
    static emscripten::val suspendResumeControlJs();

    void suspend();
    int sendPendingEvents();

private:
    friend void qtSendPendingEvents();

    static QWasmSuspendResumeControl *s_suspendResumeControl;
    std::map<int, std::function<void(emscripten::val)>> m_eventHandlers;
};

class Q_CORE_EXPORT QWasmEventHandler
{
public:
    QWasmEventHandler() = default;
    QWasmEventHandler(emscripten::val element, const std::string &name,
                      std::function<void(emscripten::val)> fn);
    ~QWasmEventHandler();
    QWasmEventHandler(QWasmEventHandler const&) = delete;
    QWasmEventHandler& operator=(QWasmEventHandler const&) = delete;
    QWasmEventHandler(QWasmEventHandler&& other) noexcept;
    QWasmEventHandler& operator=(QWasmEventHandler&& other) noexcept;
private:
    emscripten::val m_element;
    emscripten::val m_name;
    uint32_t m_eventHandlerIndex = 0;
};

class QWasmTimer
{
public:
    QWasmTimer(QWasmSuspendResumeControl* suspendResume, std::function<void()> handler);
    ~QWasmTimer();
    QWasmTimer(QWasmTimer const&) = delete;
    QWasmTimer& operator=(QWasmTimer const&) = delete;
    void setTimeout(std::chrono::milliseconds timeout);
    bool hasTimeout();
    void clearTimeout();

private:
    QWasmSuspendResumeControl *m_suspendResume;
    uint32_t m_handlerIndex;
    uint64_t m_timerId = 0;
};

#endif
