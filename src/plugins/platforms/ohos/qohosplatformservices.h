// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMSERVICES_H
#define QOHOSPLATFORMSERVICES_H

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformServices : public QPlatformServices
{
public:
    QOhosPlatformServices();

    bool hasCapability(Capability capability) const override;
    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
    QByteArray desktopEnvironment() const override;

    QPlatformServiceColorPicker *colorPicker(QWindow *parent) override;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMSERVICES_H
