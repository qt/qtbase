// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QWINDOWSGDINATIVEINTERFACE_H
#define QWINDOWSGDINATIVEINTERFACE_H

#include "qwindowsnativeinterface.h"

QT_BEGIN_NAMESPACE

class QWindowsGdiNativeInterface : public QWindowsNativeInterface
{
    Q_OBJECT
public:
    void *nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs) override;
};

QT_END_NAMESPACE

#endif // QWINDOWSGDINATIVEINTERFACE_H
