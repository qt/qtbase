// Copyright (C) 2024 The Qt Company Ltd.
// Copyright © 2004-2023 Unicode, Inc.
// SPDX-License-Identifier: Unicode-3.0

#ifndef QTIMEZONELOCALE_DATA_P_H
#define QTIMEZONELOCALE_DATA_P_H

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

// Only qtimezonelocale.cpp should #include this (after other things it needs),
// and even that only when feature icu is disabled.
#include "qtimezonelocale_p.h"

QT_REQUIRE_CONFIG(timezone_locale);
#if QT_CONFIG(icu)
#  error "This file should only be needed (or seen) when ICU is not in use"
#endif

QT_BEGIN_NAMESPACE

namespace QtTimeZoneLocale {

// GENERATED PART STARTS HERE

// GENERATED PART ENDS HERE

} // QtTimeZoneLocale

QT_END_NAMESPACE

#endif // QTIMEZONELOCALE_DATA_P_H
