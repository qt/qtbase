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

#ifndef QPRINTENGINE_PDF_P_H
#define QPRINTENGINE_PDF_P_H

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

#include "QtPrintSupport/qprintengine.h"

#ifndef QT_NO_PRINTER
#include "QtCore/qmap.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qpainterpath.h"
#include "QtCore/qdatastream.h"

#include "private/qfontengine_p.h"
#include "private/qpdf_p.h"
#include "private/qpaintengine_p.h"
#include "qprintengine.h"
#include "qprint_p.h"

QT_BEGIN_NAMESPACE

class QImage;
class QDataStream;
class QPen;
class QPointF;
class QRegion;
class QFile;

class QPdfPrintEnginePrivate;

class Q_PRINTSUPPORT_EXPORT QPdfPrintEngine : public QPdfEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QPdfPrintEngine)
public:
    QPdfPrintEngine(QPrinter::PrinterMode m, QPdfEngine::PdfVersion version = QPdfEngine::Version_1_4);
    virtual ~QPdfPrintEngine();

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev) override;
    bool end() override;
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() override {return false;}
    QPrinter::PrinterState printerState() const override {return state;}

    bool newPage() override;
    int metric(QPaintDevice::PaintDeviceMetric) const override;
    virtual void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
    virtual QVariant property(PrintEnginePropertyKey key) const override;
    // end reimplementations QPrintEngine

    QPrinter::PrinterState state;

protected:
    QPdfPrintEngine(QPdfPrintEnginePrivate &p);

private:
    Q_DISABLE_COPY(QPdfPrintEngine)
};

class Q_PRINTSUPPORT_EXPORT QPdfPrintEnginePrivate : public QPdfEnginePrivate
{
    Q_DECLARE_PUBLIC(QPdfPrintEngine)
public:
    QPdfPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QPdfPrintEnginePrivate();

    virtual bool openPrintDevice();
    virtual void closePrintDevice();

private:
    Q_DISABLE_COPY(QPdfPrintEnginePrivate)

    friend class QCupsPrintEngine;
    friend class QCupsPrintEnginePrivate;

    QString printerName;
    QString printProgram;
    QString selectionOption;

    bool collate;
    int copies;
    QPrinter::PageOrder pageOrder;
    QPrinter::PaperSource paperSource;

    int fd;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_PDF_P_H
