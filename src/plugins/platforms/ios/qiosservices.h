// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSSERVICES_H
#define QIOSSERVICES_H

#include <qurl.h>
#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QIOSServices : public QPlatformServices
{
public:
    bool openUrl(const QUrl &url);
    bool openDocument(const QUrl &url);

    bool handleUrl(const QUrl &url);

private:
    QUrl m_handlingUrl;
};

QT_END_NAMESPACE

#endif // QIOSSERVICES_H
