// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCPDBSUPPORT_H
#define QCPDBSUPPORT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include <QtPrintSupport/private/qprint_p.h>
#include <QHash>
#include <QLocale>

#include <cpdb/frontend.h>

QT_REQUIRE_CONFIG(cpdb);

QT_BEGIN_NAMESPACE

class QPrintDevice;

#define PDPK_CpdbPrinterObj QPrintDevice::PrintDevicePropertyKey(QPrintDevice::PDPK_CustomBase + 0x10)

class QCPDBSupport
{
public:
    constexpr static qreal pointsMultiplier = 2.83464566929; // 1mm to points

    static const char *translateOption(cpdb_printer_obj_t *printerObj, const char *optionName);
    static const char *translateChoice(cpdb_printer_obj_t *printerObj, const char *optionName, const char *choiceName);
    static const char *translateGroup(cpdb_printer_obj_t *printerObj, const char *groupName);

    const static inline QHash<QByteArray,QByteArray> numUpDist = {
        {"1", "1x1"},
        {"2", "2x1"},
        {"4", "2x2"},
        {"6", "2x3"},
        {"9", "3x3"},
        {"16", "4x4"},

        {"1x1", "1"},
        {"2x1", "2"},
        {"2x2", "4"},
        {"2x3", "6"},
        {"3x3", "9"},
        {"4x4", "16"},
    };

    const static inline QHash<QByteArray,QPrint::DuplexMode> duplexMap = {
        {CPDB_SIDES_ONE_SIDED, QPrint::DuplexNone},
        {CPDB_SIDES_TWO_SIDED_SHORT, QPrint::DuplexShortSide},
        {CPDB_SIDES_TWO_SIDED_LONG, QPrint::DuplexLongSide}
    };

    const static inline QHash<QPrint::DuplexMode,QByteArray> qDuplexMap = {
        {QPrint::DuplexNone, CPDB_SIDES_ONE_SIDED},
        {QPrint::DuplexShortSide, CPDB_SIDES_TWO_SIDED_SHORT},
        {QPrint::DuplexLongSide, CPDB_SIDES_TWO_SIDED_LONG}
    };
};


QT_END_NAMESPACE

#endif // QCPDBSUPPORT_H
