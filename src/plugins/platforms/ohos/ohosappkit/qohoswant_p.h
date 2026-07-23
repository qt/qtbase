// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWANT_P_H
#define QOHOSWANT_P_H

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

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtOhosAppKit/private/qohossharekit_p.h>
#include <QtOhosAppKit/private/qtohosappkitglobal_p.h>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

enum class WantFlag {
    AuthReadUriPermission = 1 << 0,
    AuthWriteUriPermission = 1 << 1,
    InstallOnDemand = 1 << 2,
};
Q_DECLARE_FLAGS(WantFlags, WantFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(WantFlags)

struct Want
{
    QString deviceId;
    QString bundleName;
    QString moduleName;
    QString abilityName;
    QString uri;
    QString type;
    QString action;
    QStringList entities;
    WantFlags flags;
    QJsonObject parameters;
    QHash<QString, int> fds;
};

class Q_OHOSAPPKIT_EXPORT WantInfo
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

    virtual ~WantInfo();

    virtual Want want() const = 0;

    virtual std::optional<QList<std::shared_ptr<ShareKit::SharedRecord>>> tryGetSharedRecordsFromShareKit() const = 0;

    virtual std::optional<ContactInfo> tryGetContactInfo() const = 0;

    virtual LaunchReason launchReason() const = 0;

protected:
    WantInfo();

private:
    Q_DISABLE_COPY(WantInfo)
};

}

QT_END_NAMESPACE

#endif
