// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMDESKTOPSERVICES_H
#define QWASMDESKTOPSERVICES_H

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QWasmServices : public QPlatformServices
{
public:
    bool openUrl(const QUrl &url) override;
};

QT_END_NAMESPACE

#endif // QWASMDESKTOPSERVICES_H
