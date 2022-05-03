// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDATETIME_P_H
#define QDATETIME_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "qplatformdefs.h"
#include "QtCore/qatomic.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qshareddata.h"

#if QT_CONFIG(timezone)
#include "qtimezone.h"
#endif

QT_BEGIN_NAMESPACE

class QDateTimePrivate : public QSharedData
{
public:
    // forward the declarations from QDateTime (this makes them public)
    typedef QDateTime::ShortData QDateTimeShortData;
    typedef QDateTime::Data QDateTimeData;

    // Never change or delete this enum, it is required for backwards compatible
    // serialization of QDateTime before 5.2, so is essentially public API
    enum Spec {
        LocalUnknown = -1,
        LocalStandard = 0,
        LocalDST = 1,
        UTC = 2,
        OffsetFromUTC = 3,
        TimeZone = 4
    };

    // Daylight Time Status
    enum DaylightStatus {
        UnknownDaylightTime = -1,
        StandardTime = 0,
        DaylightTime = 1
    };

    // Status of date/time
    enum StatusFlag {
        ShortData           = 0x01,

        ValidDate           = 0x02,
        ValidTime           = 0x04,
        ValidDateTime       = 0x08,
        ValidWhenMask       = ValidDate | ValidTime | ValidDateTime,

        TimeSpecMask        = 0x30,

        SetToStandardTime   = 0x40,
        SetToDaylightTime   = 0x80,
        ValidityMask        = ValidDate | ValidTime | ValidDateTime,
        DaylightMask        = SetToStandardTime | SetToDaylightTime,
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    enum {
        TimeSpecShift = 4,
    };

    struct ZoneState {
        qint64 when; // ms after zone/local 1970 start
        int offset = 0; // seconds
        DaylightStatus dst = UnknownDaylightTime;
        bool valid = false;

        ZoneState(qint64 local) : when(local) {}
        ZoneState(qint64 w, int o, DaylightStatus d, bool v = true)
            : when(w), offset(o), dst(d), valid(v) {}
    };

    static QDateTime::Data create(QDate toDate, QTime toTime, Qt::TimeSpec toSpec,
                                  int offsetSeconds);

#if QT_CONFIG(timezone)
    static QDateTime::Data create(QDate toDate, QTime toTime, const QTimeZone & timeZone);

    static qint64 zoneMSecsToEpochMSecs(qint64 msecs, const QTimeZone &zone,
                                        DaylightStatus *hint = nullptr,
                                        QDate *localDate = nullptr, QTime *localTime = nullptr,
                                        QString *abbreviation = nullptr);
#endif // timezone

    static ZoneState expressUtcAsLocal(qint64 utcMSecs);

    static qint64 localMSecsToEpochMSecs(qint64 localMsecs,
                                         DaylightStatus *daylightStatus,
                                         QDate *localDate = nullptr, QTime *localTime = nullptr,
                                         QString *abbreviation = nullptr);

    StatusFlags m_status = StatusFlag(Qt::LocalTime << TimeSpecShift);
    qint64 m_msecs = 0;
    int m_offsetFromUtc = 0;
#if QT_CONFIG(timezone)
    QTimeZone m_timeZone;
#endif // timezone
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimePrivate::StatusFlags)

QT_END_NAMESPACE

#endif // QDATETIME_P_H
