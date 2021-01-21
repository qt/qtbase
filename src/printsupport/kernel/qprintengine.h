/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPRINTENGINE_H
#define QPRINTENGINE_H

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QtCore/qvariant.h>
#include <QtPrintSupport/qprinter.h>

Q_DECLARE_METATYPE(QMarginsF)

QT_BEGIN_NAMESPACE


#ifndef QT_NO_PRINTER

class Q_PRINTSUPPORT_EXPORT QPrintEngine
{
public:
    virtual ~QPrintEngine() {}
    enum PrintEnginePropertyKey {
        PPK_CollateCopies,
        PPK_ColorMode,
        PPK_Creator,
        PPK_DocumentName,
        PPK_FullPage,
        PPK_NumberOfCopies,
        PPK_Orientation,
        PPK_OutputFileName,
        PPK_PageOrder,
        PPK_PageRect,
        PPK_PageSize,
        PPK_PaperRect,
        PPK_PaperSource,
        PPK_PrinterName,
        PPK_PrinterProgram,
        PPK_Resolution,
        PPK_SelectionOption,
        PPK_SupportedResolutions,

        PPK_WindowsPageSize,
        PPK_FontEmbedding,

        PPK_Duplex,

        PPK_PaperSources,
        PPK_CustomPaperSize,
        PPK_PageMargins,
        PPK_CopyCount,
        PPK_SupportsMultipleCopies,
        PPK_PaperName,
        PPK_QPageSize,
        PPK_QPageMargins,
        PPK_QPageLayout,
        PPK_PaperSize = PPK_PageSize,

        PPK_CustomBase = 0xff00
    };

    virtual void setProperty(PrintEnginePropertyKey key, const QVariant &value) = 0;
    virtual QVariant property(PrintEnginePropertyKey key) const = 0;

    virtual bool newPage() = 0;
    virtual bool abort() = 0;

    virtual int metric(QPaintDevice::PaintDeviceMetric) const = 0;

    virtual QPrinter::PrinterState printerState() const = 0;
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPRINTENGINE_H
