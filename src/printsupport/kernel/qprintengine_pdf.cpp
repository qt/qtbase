/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qprintengine_pdf_p.h"

#ifndef QT_NO_PRINTER

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "qprinterinfo.h"

#include <limits.h>
#include <math.h>


#ifdef Q_OS_UNIX
#include "private/qcore_unix_p.h" // overrides QT_OPEN
#endif

#ifdef Q_OS_WIN
#include <io.h> // _close.
#endif

QT_BEGIN_NAMESPACE

//#define FONT_DUMP

extern QSizeF qt_paperSizeToQSizeF(QPrinter::PaperSize size);

#define Q_MM(n) int((n * 720 + 127) / 254)
#define Q_IN(n) int(n * 72)

static const char * const psToStr[QPrinter::NPageSize+1] =
{
    "A4", "B5", "Letter", "Legal", "Executive",
    "A0", "A1", "A2", "A3", "A5", "A6", "A7", "A8", "A9", "B0", "B1",
    "B10", "B2", "B3", "B4", "B6", "B7", "B8", "B9", "C5E", "Comm10E",
    "DLE", "Folio", "Ledger", "Tabloid", 0
};

QPdf::PaperSize QPdf::paperSize(QPrinter::PaperSize paperSize)
{
    QSizeF s = qt_paperSizeToQSizeF(paperSize);
    PaperSize p = { Q_MM(s.width()), Q_MM(s.height()) };
    return p;
}

const char *QPdf::paperSizeToString(QPrinter::PaperSize paperSize)
{
    return psToStr[paperSize];
}


QPdfPrintEngine::QPdfPrintEngine(QPrinter::PrinterMode m)
    : QPdfEngine(*new QPdfPrintEnginePrivate(m))
{
    state = QPrinter::Idle;
}

QPdfPrintEngine::QPdfPrintEngine(QPdfPrintEnginePrivate &p)
    : QPdfEngine(p)
{
    state = QPrinter::Idle;
}

QPdfPrintEngine::~QPdfPrintEngine()
{
}

bool QPdfPrintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QPdfPrintEngine);

    if (!d->openPrintDevice()) {
        state = QPrinter::Error;
        return false;
    }
    state = QPrinter::Active;

    return QPdfEngine::begin(pdev);
}

bool QPdfPrintEngine::end()
{
    Q_D(QPdfPrintEngine);

    QPdfEngine::end();

    d->closePrintDevice();
    state = QPrinter::Idle;

    return true;
}

bool QPdfPrintEngine::newPage()
{
    return QPdfEngine::newPage();
}

int QPdfPrintEngine::metric(QPaintDevice::PaintDeviceMetric m) const
{
    return QPdfEngine::metric(m);
}

void QPdfPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QPdfPrintEngine);

    switch (int(key)) {
    case PPK_CollateCopies:
        d->collate = value.toBool();
        break;
    case PPK_ColorMode:
        d->grayscale = (QPrinter::ColorMode(value.toInt()) == QPrinter::GrayScale);
        break;
    case PPK_Creator:
        d->creator = value.toString();
        break;
    case PPK_DocumentName:
        d->title = value.toString();
        break;
    case PPK_FullPage:
        d->fullPage = value.toBool();
        break;
    case PPK_CopyCount: // fallthrough
    case PPK_NumberOfCopies:
        d->copies = value.toInt();
        break;
    case PPK_Orientation:
        d->landscape = (QPrinter::Orientation(value.toInt()) == QPrinter::Landscape);
        break;
    case PPK_OutputFileName:
        d->outputFileName = value.toString();
        break;
    case PPK_PageOrder:
        d->pageOrder = QPrinter::PageOrder(value.toInt());
        break;
    case PPK_PaperSize:
        d->printerPaperSize = QPrinter::PaperSize(value.toInt());
        d->updatePaperSize();
        break;
    case PPK_PaperSource:
        d->paperSource = QPrinter::PaperSource(value.toInt());
        break;
    case PPK_PrinterName:
        d->printerName = value.toString();
        break;
    case PPK_PrinterProgram:
        d->printProgram = value.toString();
        break;
    case PPK_Resolution:
        d->resolution = value.toInt();
        break;
    case PPK_SelectionOption:
        d->selectionOption = value.toString();
        break;
    case PPK_FontEmbedding:
        d->embedFonts = value.toBool();
        break;
    case PPK_Duplex:
        d->duplex = static_cast<QPrinter::DuplexMode> (value.toInt());
        break;
    case PPK_CustomPaperSize:
        d->printerPaperSize = QPrinter::Custom;
        d->customPaperSize = value.toSizeF();
        d->updatePaperSize();
        break;
    case PPK_PageMargins:
    {
        QList<QVariant> margins(value.toList());
        Q_ASSERT(margins.size() == 4);
        d->leftMargin = margins.at(0).toReal();
        d->topMargin = margins.at(1).toReal();
        d->rightMargin = margins.at(2).toReal();
        d->bottomMargin = margins.at(3).toReal();
        d->pageMarginsSet = true;
        break;
    }
    default:
        break;
    }
}

QVariant QPdfPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QPdfPrintEngine);

    QVariant ret;
    switch (int(key)) {
    case PPK_CollateCopies:
        ret = d->collate;
        break;
    case PPK_ColorMode:
        ret = d->grayscale ? QPrinter::GrayScale : QPrinter::Color;
        break;
    case PPK_Creator:
        ret = d->creator;
        break;
    case PPK_DocumentName:
        ret = d->title;
        break;
    case PPK_FullPage:
        ret = d->fullPage;
        break;
    case PPK_CopyCount:
        ret = d->copies;
        break;
    case PPK_SupportsMultipleCopies:
        ret = false;
        break;
    case PPK_NumberOfCopies:
        ret = d->copies;
        break;
    case PPK_Orientation:
        ret = d->landscape ? QPrinter::Landscape : QPrinter::Portrait;
        break;
    case PPK_OutputFileName:
        ret = d->outputFileName;
        break;
    case PPK_PageOrder:
        ret = d->pageOrder;
        break;
    case PPK_PaperSize:
        ret = d->printerPaperSize;
        break;
    case PPK_PaperSource:
        ret = d->paperSource;
        break;
    case PPK_PrinterName:
        ret = d->printerName;
        break;
    case PPK_PrinterProgram:
        ret = d->printProgram;
        break;
    case PPK_Resolution:
        ret = d->resolution;
        break;
    case PPK_SupportedResolutions:
        ret = QList<QVariant>() << 72;
        break;
    case PPK_PaperRect:
        ret = d->paperRect();
        break;
    case PPK_PageRect:
        ret = d->pageRect();
        break;
    case PPK_SelectionOption:
        ret = d->selectionOption;
        break;
    case PPK_FontEmbedding:
        ret = d->embedFonts;
        break;
    case PPK_Duplex:
        ret = d->duplex;
        break;
    case PPK_CustomPaperSize:
        ret = d->customPaperSize;
        break;
    case PPK_PageMargins:
    {
        QList<QVariant> margins;
        if (d->printerPaperSize == QPrinter::Custom && !d->pageMarginsSet)
            margins << 0 << 0 << 0 << 0;
        else
            margins << d->leftMargin << d->topMargin
                    << d->rightMargin << d->bottomMargin;
        ret = margins;
        break;
    }
    default:
        break;
    }
    return ret;
}


bool QPdfPrintEnginePrivate::openPrintDevice()
{
    if (outDevice)
        return false;

    if (!outputFileName.isEmpty()) {
        QFile *file = new QFile(outputFileName);
        if (! file->open(QFile::WriteOnly|QFile::Truncate)) {
            delete file;
            return false;
        }
        outDevice = file;
    }

    return true;
}

void QPdfPrintEnginePrivate::closePrintDevice()
{
    if (outDevice) {
        outDevice->close();
        if (fd >= 0)
    #if defined(Q_OS_WIN) && defined(_MSC_VER) && _MSC_VER >= 1400
            ::_close(fd);
    #else
            ::close(fd);
    #endif
        fd = -1;
        delete outDevice;
        outDevice = 0;
    }
}



QPdfPrintEnginePrivate::QPdfPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfEnginePrivate(),
      duplex(QPrinter::DuplexNone),
      collate(false),
      copies(1),
      pageOrder(QPrinter::FirstPageFirst),
      paperSource(QPrinter::Auto),
      printerPaperSize(QPrinter::A4),
      pageMarginsSet(false),
      fd(-1)
{
    resolution = 72;
    if (m == QPrinter::HighResolution)
        resolution = 1200;
    else if (m == QPrinter::ScreenResolution)
        resolution = qt_defaultDpi();
}

QPdfPrintEnginePrivate::~QPdfPrintEnginePrivate()
{
}


void QPdfPrintEnginePrivate::updatePaperSize()
{
    if (printerPaperSize == QPrinter::Custom) {
        paperSize = customPaperSize;
    } else {
        QPdf::PaperSize s = QPdf::paperSize(printerPaperSize);
        paperSize = QSize(s.width, s.height);
    }
}


QT_END_NAMESPACE

#endif // QT_NO_PRINTER
