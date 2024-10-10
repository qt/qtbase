// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMEZONELOCALE_P_H
#define QTIMEZONELOCALE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//
#include <private/qglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qtimezone.h>

#if QT_CONFIG(icu)
#include <unicode/ucal.h>
#endif

QT_REQUIRE_CONFIG(timezone);
QT_REQUIRE_CONFIG(timezone_locale);

QT_BEGIN_NAMESPACE

namespace QtTimeZoneLocale {
#if QT_CONFIG(icu)
QString ucalTimeZoneDisplayName(UCalendar *ucal, QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QByteArray &localeCode);
#else
// Define data types for QTZL_data_p.h

#endif
}

QT_END_NAMESPACE

#endif // QTIMEZONELOCALE_P_H
