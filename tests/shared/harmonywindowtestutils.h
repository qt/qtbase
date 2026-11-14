// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QT_TESTS_SHARED_HARMONY_WINDOW_TEST_UTILS_H
#define QT_TESTS_SHARED_HARMONY_WINDOW_TEST_UTILS_H

#include "harmonytestutils.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qobject.h>
#include <QtGui/qwindow.h>

#include <optional>

namespace QTestHarmony {

inline std::optional<bool> tryFetchJsWindowIsInFreeWindowMode(const QHarmonyJsObjectRef &jsWindowRef)
{
    return QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            return tryMapJsBooleanToBool(env, callJsMethod(env, jsWindowRef.value(jsContext), "isInFreeWindowMode"));
        });
}

inline std::optional<bool> tryFetchIsJsWindowFloating(const QHarmonyJsObjectRef &jsWindowRef)
{
    return QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) -> std::optional<bool> {
            const auto env = jsContext.env();
            const auto optStatus = tryMapJsNumberToInt(env, callJsMethod(env, jsWindowRef.value(jsContext), "getWindowStatus"));
            const auto optFloatingStatus = tryGetJsEnumerator(jsContext, "@ohos.window", "WindowStatusType", "FLOATING");
            if (!optStatus || !optFloatingStatus)
                return std::nullopt;
            return *optStatus == *optFloatingStatus;
        });
}

// QWindow::setWindowStates() sends no QEvent::WindowStateChange, so every such event comes
// from the platform plugin.
class WindowStateReportCounter : public QObject
{
public:
    explicit WindowStateReportCounter(QWindow *window);

    int maximizedReportCount() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWindow *m_window;
    int m_maximizedReportCount = 0;
};

inline WindowStateReportCounter::WindowStateReportCounter(QWindow *window)
    : m_window(window)
{
    window->installEventFilter(this);
}

inline int WindowStateReportCounter::maximizedReportCount() const
{
    return m_maximizedReportCount;
}

inline bool WindowStateReportCounter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::WindowStateChange
        && m_window->windowStates().testFlag(Qt::WindowMaximized)) {
        ++m_maximizedReportCount;
    }
    return false;
}

}

#endif // QT_TESTS_SHARED_HARMONY_WINDOW_TEST_UTILS_H
