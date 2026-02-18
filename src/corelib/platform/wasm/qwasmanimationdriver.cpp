// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwasmanimationdriver_p.h"
#include "qwasmsuspendresumecontrol_p.h"

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE


// QWasmAnimationDriver drives animations using requestAnimationFrame(). This
// ensures that animations are advanced in sync with frame update calls, which
// again are synced to the screen refresh rate.

namespace {
    constexpr int FallbackTimerInterval = 500;
}

QWasmAnimationDriver::QWasmAnimationDriver(QUnifiedTimer *)
    : QAnimationDriver(nullptr)
{
    connect(this, &QAnimationDriver::started, this, &QWasmAnimationDriver::start);
    connect(this, &QAnimationDriver::stopped, this, &QWasmAnimationDriver::stop);
}

QWasmAnimationDriver::~QWasmAnimationDriver()
{
    disconnect(this, &QAnimationDriver::started, this, &QWasmAnimationDriver::start);
    disconnect(this, &QAnimationDriver::stopped, this, &QWasmAnimationDriver::stop);

    if (m_animateCallbackHandle != 0)
        QWasmAnimationFrameMultiHandler::instance()->unregisterAnimateCallback(m_animateCallbackHandle);
}

qint64 QWasmAnimationDriver::elapsed() const
{
    return isRunning() ? qint64(m_currentTimestamp - m_startTimestamp) : 0;
}

double QWasmAnimationDriver::getCurrentTimeFromTimeline() const
{
    // Get the current timeline time, which is an equivalent time source to the
    // animation frame time source. According to the documentation this API
    // may be unavailable in various cases; check for null before accessing.
    emscripten::val document = emscripten::val::global("document");
    emscripten::val timeline = document["timeline"];
    if (!timeline.isNull() && !timeline.isUndefined()) {
        emscripten::val currentTime = timeline["currentTime"];
        if (!currentTime.isNull() && !currentTime.isUndefined())
            return currentTime.as<double>();
    }
    return 0;
}

void QWasmAnimationDriver::handleFallbackTimeout()
{
    if (!isRunning())
        return;

    // Get the current time from a timing source equivalent to the animation frame time
    double currentTime = getCurrentTimeFromTimeline();
    if (currentTime == 0)
        currentTime = m_currentTimestamp + FallbackTimerInterval;
    const double timeSinceLastFrame = currentTime - m_currentTimestamp;

    // Advance animations if animations are active but there has been no rcent animation
    // frame callback.
    if (timeSinceLastFrame > FallbackTimerInterval * 0.8) {
        m_currentTimestamp = currentTime;
        advance();
    }
}

void QWasmAnimationDriver::start()
{
    if (isRunning())
        return;

    // Set start timestamp to document.timeline.currentTime()
    m_startTimestamp = getCurrentTimeFromTimeline();
    m_currentTimestamp = m_startTimestamp;

    // Register animate callback
    m_animateCallbackHandle = QWasmAnimationFrameMultiHandler::instance()->registerAnimateCallback(
        [this](double timestamp) { handleAnimationFrame(timestamp); });

    // Start fallback timer to ensure animations advance even if animaton frame callbacks stop coming
    fallbackTimer.setInterval(FallbackTimerInterval);
    connect(&fallbackTimer, &QTimer::timeout, this, &QWasmAnimationDriver::handleFallbackTimeout);
    fallbackTimer.start();

    QAnimationDriver::start();
}

void QWasmAnimationDriver::stop()
{
    m_startTimestamp = 0;
    m_currentTimestamp = 0;

    // Stop and disconnect the fallback timer
    fallbackTimer.stop();
    disconnect(&fallbackTimer, &QTimer::timeout, this, &QWasmAnimationDriver::handleFallbackTimeout);

    // Deregister the animation frame callback
    if (m_animateCallbackHandle != 0) {
        QWasmAnimationFrameMultiHandler::instance()->unregisterAnimateCallback(m_animateCallbackHandle);
        m_animateCallbackHandle = 0;
    }

    QAnimationDriver::stop();
}

void QWasmAnimationDriver::handleAnimationFrame(double timestamp)
{
    if (!isRunning())
        return;

    m_currentTimestamp = timestamp;

    // Fall back to setting m_startTimestamp here in cases where currentTime
    // was not available in start() (gives 0 elapsed time for the first frame)
    if (m_startTimestamp == 0)
        m_startTimestamp = timestamp;

    advance();
}

QT_END_NAMESPACE
