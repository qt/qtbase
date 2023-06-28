// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMEVENTDISPATCHER_H
#define QWASMEVENTDISPATCHER_H

#include <QtCore/private/qeventdispatcher_wasm_p.h>

QT_BEGIN_NAMESPACE

class QWasmEventDispatcher : public QEventDispatcherWasm
{
protected:
    bool processPostedEvents() override;
};

QT_END_NAMESPACE

#endif
