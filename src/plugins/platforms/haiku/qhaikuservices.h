// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QHAIKUSERVICES_H
#define QHAIKUSERVICES_H

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QHaikuServices : public QPlatformServices
{
public:
    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;

    QByteArray desktopEnvironment() const override;
};

QT_END_NAMESPACE

#endif
