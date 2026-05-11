// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSCLOSEEVENTCONTEXT_P_H
#define QOHOSCLOSEEVENTCONTEXT_P_H

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

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qglobal.h>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

class QOhosCloseEventContext
{
public:
    enum class CloseRootCause
    {
        NotSpecified,
        OnPrepareToTerminate,
        WindowStageClose,
        SubWindowClose,
    };

    enum class CloseResolution
    {
        Reject,
        Close,
    };

    static CloseRootCause getCloseRootCauseForEventOrDefault(QEvent *event);
    static void runWithCloseRootCauseSet(CloseRootCause rootCause, const std::function<void()> &task);
    static void runWithCloseRootCauseAndCloseResolutionConsumerSet(
        CloseRootCause rootCause, QOhosConsumer<CloseResolution> closeResolutionConsumer, const std::function<void()> &task);
    static void notifyCloseResolutionFromEventIfValid(QEvent *event);
};

QT_END_NAMESPACE

#endif
