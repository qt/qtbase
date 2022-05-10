// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGENERICUNIXDESKTOPSERVICES_H
#define QGENERICUNIXDESKTOPSERVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformservices.h>
#include <QtCore/QString>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QGenericUnixServices : public QPlatformServices
{
public:
    QGenericUnixServices() {}

    QByteArray desktopEnvironment() const override;

    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;

private:
    QString m_webBrowser;
    QString m_documentLauncher;
};

QT_END_NAMESPACE

#endif // QGENERICUNIXDESKTOPSERVICES_H
