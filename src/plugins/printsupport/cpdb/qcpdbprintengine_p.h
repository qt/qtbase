// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCPDBPRINTENGINE_H
#define QCPDBPRINTENGINE_H

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

#include <private/qcpdb_p.h>

#include "QtPrintSupport/qprintengine.h"

#include <QtCore/qstring.h>
#include <QtCore/qtemporaryfile.h>
#include <QtGui/qpaintengine.h>

#include <private/qpaintengine_p.h>
#include <private/qprintdevice_p.h>
#include <private/qprintengine_pdf_p.h>

QT_BEGIN_NAMESPACE

class QCpdbPrintEnginePrivate;

class QCpdbPrintEngine : public QPdfPrintEngine
{
    Q_DECLARE_PRIVATE(QCpdbPrintEngine)
public:
    QCpdbPrintEngine(QPrinter::PrinterMode m, const QString &deviceId);
    virtual ~QCpdbPrintEngine();

    // reimplementations QPdfPrintEngine
    void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    QVariant property(PrintEnginePropertyKey key) const override;
    // end reimplementations QPdfPrintEngine

private:
    Q_DISABLE_COPY_MOVE(QCpdbPrintEngine)
};

class QCpdbPrintEnginePrivate : public QPdfPrintEnginePrivate
{
    Q_DECLARE_PUBLIC(QCpdbPrintEngine)
public:
    QCpdbPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QCpdbPrintEnginePrivate();

    bool openPrintDevice() override;
    void closePrintDevice() override;

private:
    Q_DISABLE_COPY_MOVE(QCpdbPrintEnginePrivate)

    void changePrinter(const QString &newPrinter);
    void setPageSize(const QPageSize &pageSize);

    QPrintDevice m_printDevice;
    cpdb_printer_obj_t *m_printerObj = nullptr;
    QTemporaryFile *cpdbTempFile = nullptr;
    QPrint::DuplexMode duplex;
    bool duplexRequestedExplicitly = false;
};

QT_END_NAMESPACE

#endif // QCPDBPRINTENGINE_H
