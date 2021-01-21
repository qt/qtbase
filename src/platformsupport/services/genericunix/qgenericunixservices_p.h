/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QGENERICUNIXDESKTOPSERVICES_H
#define QGENERICUNIXDESKTOPSERVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformservices.h>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

class QGenericUnixServices : public QPlatformServices
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
