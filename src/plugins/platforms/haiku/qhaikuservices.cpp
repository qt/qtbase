// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
