// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWANT_H
#define QOHOSWANT_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtOhosAppKit/qohossharekit.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

enum class QOhosWantFlag {
    AuthReadUriPermission = 1 << 0,
    AuthWriteUriPermission = 1 << 1,
    InstallOnDemand = 1 << 2,
};
Q_DECLARE_FLAGS(QOhosWantFlags, QOhosWantFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(QOhosWantFlags)

struct QOhosWant
{
    QString deviceId;
    QString bundleName;
    QString moduleName;
    QString abilityName;
    QString uri;
    QString type;
    QString action;
    QStringList entities;
    QOhosWantFlags flags;
    QJsonObject parameters;
    QHash<QString, int> fds;
};

class Q_OHOSAPPKIT_EXPORT QOhosWantInfo
{
public:
    enum class LaunchReason {
        Unknown,
        StartAbility,
        Continuation,
        PrepareContinuation,
        Preload,
    };

    struct ContactInfo
    {
        QString contactType;
        QString contactId;
    };

    virtual ~QOhosWantInfo();

    virtual QOhosWant want() const = 0;

    virtual QSharedPointer<QList<QSharedPointer<ShareKit::SharedRecord>>> tryGetSharedRecordsFromShareKit() const = 0;

    virtual QSharedPointer<ContactInfo> tryGetContactInfo() const = 0;

    virtual LaunchReason launchReason() const = 0;

protected:
    QOhosWantInfo();

private:
    Q_DISABLE_COPY(QOhosWantInfo)
};

}

QT_END_NAMESPACE

#endif
