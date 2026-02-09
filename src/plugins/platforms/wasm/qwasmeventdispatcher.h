// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMEVENTDISPATCHER_H
#define QWASMEVENTDISPATCHER_H

#include <QtCore/private/qeventdispatcher_wasm_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QWasmSuspendResumeControl;

class QWasmEventDispatcher : public QEventDispatcherWasm
{
public:
    QWasmEventDispatcher(std::shared_ptr<QWasmSuspendResumeControl> suspendResume);

protected:
    bool sendPostedEvents() override;
    void onLoaded() override;
};

QT_END_NAMESPACE

#endif
