// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSERVICES_H
#define QPLATFORMSERVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

class QUrl;

class Q_GUI_EXPORT QPlatformServices
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformServices)

    QPlatformServices();
    virtual ~QPlatformServices() { }

    virtual bool openUrl(const QUrl &url);
    virtual bool openDocument(const QUrl &url);

    virtual QByteArray desktopEnvironment() const;
};

QT_END_NAMESPACE

#endif // QPLATFORMSERVICES_H
