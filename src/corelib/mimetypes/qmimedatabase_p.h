/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QMIMEDATABASE_P_H
#define QMIMEDATABASE_P_H

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

#include "qmimetype.h"

QT_REQUIRE_CONFIG(mimetype);

#include "qmimetype_p.h"
#include "qmimeglobpattern_p.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QIODevice;
class QMimeDatabase;
class QMimeProviderBase;

class QMimeDatabasePrivate
{
public:
    Q_DISABLE_COPY_MOVE(QMimeDatabasePrivate)

    QMimeDatabasePrivate();
    ~QMimeDatabasePrivate();

    static QMimeDatabasePrivate *instance();

    inline QString defaultMimeType() const { return m_defaultMimeType; }

    bool inherits(const QString &mime, const QString &parent);

    QList<QMimeType> allMimeTypes();

    QString resolveAlias(const QString &nameOrAlias);
    QStringList parents(const QString &mimeName);
    QMimeType mimeTypeForName(const QString &nameOrAlias);
    QMimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device, int *priorityPtr);
    QMimeType findByData(const QByteArray &data, int *priorityPtr);
    QStringList mimeTypeForFileName(const QString &fileName);
    QMimeGlobMatchResult findByFileName(const QString &fileName);

    // API for QMimeType. Takes care of locking the mutex.
    void loadMimeTypePrivate(QMimeTypePrivate &mimePrivate);
    void loadGenericIcon(QMimeTypePrivate &mimePrivate);
    void loadIcon(QMimeTypePrivate &mimePrivate);
    QStringList mimeParents(const QString &mimeName);
    QStringList listAliases(const QString &mimeName);
    bool mimeInherits(const QString &mime, const QString &parent);

private:
    using Providers = std::vector<std::unique_ptr<QMimeProviderBase>>;
    const Providers &providers();
    bool shouldCheck();
    void loadProviders();

    mutable Providers m_providers;
    QElapsedTimer m_lastCheck;

public:
    const QString m_defaultMimeType;
    QMutex mutex;
};

QT_END_NAMESPACE

#endif // QMIMEDATABASE_P_H
