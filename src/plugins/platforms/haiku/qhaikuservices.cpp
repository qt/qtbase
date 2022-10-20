/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
