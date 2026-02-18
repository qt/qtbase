// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMANIMATIONDRIVER_P_H
#define QWASMANIMATIONDRIVER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qabstractanimation.h>
#include <QtCore/qtimer.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QUnifiedTimer;

class Q_CORE_EXPORT QWasmAnimationDriver : public QAnimationDriver
{
    Q_OBJECT
public:
    QWasmAnimationDriver(QUnifiedTimer *unifiedTimer);
    ~QWasmAnimationDriver() override;

    qint64 elapsed() const override;

protected:
    void start() override;
    void stop() override;

private:
    void handleAnimationFrame(double timestamp);
    void handleFallbackTimeout();
    double getCurrentTimeFromTimeline() const;

    QTimer fallbackTimer;
    uint32_t m_animateCallbackHandle = 0;
    double m_startTimestamp = 0;
    double m_currentTimestamp = 0;
};

QT_END_NAMESPACE

#endif // QWASMANIMATIONDRIVER_P_H
