/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#include "qhaikuservices.h"

#include <QFile>
#include <QMimeDatabase>
#include <QString>
#include <QUrl>

#include <Roster.h>

QT_BEGIN_NAMESPACE

bool QHaikuServices::openUrl(const QUrl &url)
{
    const QMimeDatabase mimeDatabase;

    const QMimeType mimeType = mimeDatabase.mimeTypeForUrl(url);
    if (!mimeType.isValid())
        return false;

    const QByteArray mimeTypeName = mimeType.name().toLatin1();
    QByteArray urlData = url.toString().toLocal8Bit();
    char *rawUrlData = urlData.data();

    if (be_roster->Launch(mimeTypeName.constData(), 1, &rawUrlData) != B_OK)
        return false;

    return true;
}

bool QHaikuServices::openDocument(const QUrl &url)
{
    const QByteArray localPath = QFile::encodeName(url.toLocalFile());

    entry_ref ref;
    if (get_ref_for_path(localPath.constData(), &ref) != B_OK)
        return false;

    if (be_roster->Launch(&ref) != B_OK)
        return false;

    return true;
}

QByteArray QHaikuServices::desktopEnvironment() const
{
    return QByteArray("Haiku");
}

QT_END_NAMESPACE
