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

#include <cpdb/frontend.h>

QT_REQUIRE_CONFIG(cpdb);

QT_BEGIN_NAMESPACE

class QPrintDevice;

#define PDPK_CpdbPrinterObj QPrintDevice::PrintDevicePropertyKey(QPrintDevice::PDPK_CustomBase + 0x10)

class Q_PRINTSUPPORT_EXPORT QCPDBSupport
{
public:
    constexpr static qreal pointsMultiplier = 2.83464566929; // 1mm to points

    static const char *translateOption(cpdb_printer_obj_t *printerObj, const char *optionName);
    static const char *translateChoice(cpdb_printer_obj_t *printerObj, const char *optionName, const char *choiceName);
    static const char *translateGroup(cpdb_printer_obj_t *printerObj, const char *groupName);
};

QT_END_NAMESPACE

#endif // QCPDBSUPPORT_H
