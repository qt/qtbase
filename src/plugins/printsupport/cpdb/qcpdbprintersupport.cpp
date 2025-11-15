// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcpdbprintersupport_p.h"

#include "qcpdbprintengine_p.h"
#include "qcpdbprintdevice.h"

#include <QTime>
#include <QCoreApplication>

#include <QAbstractEventDispatcher>

QT_BEGIN_NAMESPACE

extern "C" {

static void printerUpdateCallback(cpdb_frontend_obj_t *frontendObj, cpdb_printer_obj_t *printerObj, cpdb_printer_update_t change)
{
    Q_UNUSED(frontendObj);

    switch (change) {
    case CPDB_CHANGE_PRINTER_REMOVED:
        cpdbDeletePrinterObj(printerObj);
        break;
    default:
        break;
    }
}

} // extern "C"

QCpdbPrinterSupport::QCpdbPrinterSupport()
    : QPlatformPrinterSupport()
{
    cpdbInit();
    cpdb_printer_callback printerCb = static_cast<cpdb_printer_callback>(printerUpdateCallback);

    QByteArray instanceName = "Qt";
    frontendObj = cpdbGetNewFrontendObj(instanceName.constData(), printerCb);
    cpdbIgnoreLastSavedSettings(frontendObj);
    cpdbConnectToDBus(frontendObj);
}

QCpdbPrinterSupport::~QCpdbPrinterSupport()
{
    cpdbDeleteFrontendObj(frontendObj);
}

QPrintEngine *QCpdbPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    return new QCpdbPrintEngine(printerMode, (deviceId.isEmpty() ? defaultPrintDeviceId() : deviceId));
}

QPaintEngine *QCpdbPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode);
    return static_cast<QCpdbPrintEngine *>(engine);
}

QPrintDevice QCpdbPrinterSupport::createPrintDevice(const QString &id)
{
    GHashTableIter iter;
    gpointer key, value;
    cpdb_printer_obj_t *printerObj = nullptr;
    g_hash_table_iter_init(&iter, frontendObj->printer);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        cpdb_printer_obj_t *printerObjIter = static_cast<cpdb_printer_obj_t *>(value);
        if (id == printerObjIter->id) {
            printerObj = printerObjIter;
            break;
        }
    }
    return QPlatformPrinterSupport::createPrintDevice(new QCpdbPrintDevice(printerObj));
}

QStringList QCpdbPrinterSupport::availablePrintDeviceIds() const
{
    QStringList list;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init (&iter, frontendObj->printer);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        auto printerObj = static_cast<cpdb_printer_obj_t *>(value);
        // Ignore CPDB FILE backend, since we are using Qt's native "Print To File (PDF)" printer
        if (qstrcmp(printerObj->backend_name, "FILE") != 0)
            list << printerObj->id;
    }
    return list;
}

QString QCpdbPrinterSupport::defaultPrintDeviceId() const
{
    if (cpdb_printer_obj_t *printerObj = cpdbGetDefaultPrinter(frontendObj))
        return QString(printerObj->id);

    return QString();
}

QT_END_NAMESPACE
