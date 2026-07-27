// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWANTINFO_P_H
#define QOHOSWANTINFO_P_H

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

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

namespace detail {

struct SharedRecord
{
    QString mimeType;

    std::optional<QString> content;
    std::optional<QString> filePath;

    std::optional<QString> title;
    std::optional<QString> label;
    std::optional<QString> description;
    std::optional<QByteArray> thumbnail;
    std::optional<QString> thumbnailFilePath;
    std::optional<QVariantMap> extraData;
};

class WantInfoPriv
{
public:
    enum class LaunchReason
    {
        UNKNOWN,
        START_ABILITY,
        CONTINUATION,
        PREPARE_CONTINUATION,
        PRELOAD,
    };

    struct ContactInfo
    {
        QString contactType;
        QString contactId;
    };

    virtual ~WantInfoPriv();

    virtual QJsonObject jsonObject() const = 0;

    virtual std::optional<QList<SharedRecord>> tryGetSharedDataRecords() const = 0;

    virtual std::optional<ContactInfo> tryGetContactInfo() const = 0;

    virtual LaunchReason launchReason() const = 0;

protected:
    WantInfoPriv();

private:
    Q_DISABLE_COPY(WantInfoPriv)
};

}

QSharedPointer<detail::WantInfoPriv> makeAppLaunchWantInfo();

void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer);

void addNewWantConsumer(
    QObject *context,
    QOhosConsumer<QSharedPointer<detail::WantInfoPriv>> wantConsumer);

}

QT_END_NAMESPACE

#endif // QOHOSWANTINFO_P_H
