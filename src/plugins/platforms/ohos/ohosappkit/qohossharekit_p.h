// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSHAREKIT_P_H
#define QOHOSSHAREKIT_P_H

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

#include <QtCore/qlist.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/qohossharekit.h>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit::Private {

std::shared_ptr<void> shareData(
    QWindow *optMainWindow, const QList<std::shared_ptr<ShareKit::SharedRecord>> &records,
    std::shared_ptr<ShareKit::ShareControllerOptions> controllerOptions,
    std::function<void()> panelClosedCallback,
    std::function<void(std::shared_ptr<ShareKit::ShareOperationResult>)> shareCompletedCallback);

}

QT_END_NAMESPACE

#endif
