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

#include <QtCore/qtimezone.h>

QT_REQUIRE_CONFIG(timezone);
QT_REQUIRE_CONFIG(timezone_locale);

namespace QtTimeZoneLocale {
#if QT_CONFIG(icu)
#else
// Define data types for QTZL_data_p.h

#endif
}

#endif // QTIMEZONELOCALE_P_H
