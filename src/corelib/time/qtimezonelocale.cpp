// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtimezonelocale_p.h>
#include <private/qtimezoneprivate_p.h>

#if !QT_CONFIG(icu) // Use data generated from CLDR:
#  include <private/qtimezonelocale_data_p.h>
#  include <private/qtimezoneprivate_data_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(icu) // Get data from ICU:
namespace QtTimeZoneLocale {

} // QtTimeZoneLocale
#else // No ICU, use QTZ[LP}_data_p.h data for feature timezone_locale.
namespace {
using namespace QtTimeZoneLocale; // QTZL_data_p.h
using namespace QtTimeZoneCldr; // QTZP_data_p.h
// Accessors for the QTZL_data_p.h

// Accessors for the QTZP_data_p.h

} // nameless namespace
#endif // ICU

QT_END_NAMESPACE
