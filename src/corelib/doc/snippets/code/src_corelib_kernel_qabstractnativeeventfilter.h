// Copyright (C) 2016 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QAbstractNativeEventFilter>

class MyCocoaEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override;
};
//! [0]
