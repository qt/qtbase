// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
