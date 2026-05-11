// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSINGLETHREADEXECUTOR_H
#define QOHOSSINGLETHREADEXECUTOR_H

#include <QtCore/private/qohoscommon_p.h>
#include <functional>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

struct SingleThreadExecutorConfig
{
    QOhosOptional<std::size_t> threadPreferredStackSize;
};

QOhosConsumer<std::function<void()>> makeSingleThreadExecutor(const SingleThreadExecutorConfig &config = {});

}

QT_END_NAMESPACE

#endif
