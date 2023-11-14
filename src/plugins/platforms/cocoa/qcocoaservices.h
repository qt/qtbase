// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QCOCOADESKTOPSERVICES_H
#define QCOCOADESKTOPSERVICES_H

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QCocoaServices : public QPlatformServices
{
public:
    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
};

QT_END_NAMESPACE

#endif // QCOCOADESKTOPSERVICES_H
