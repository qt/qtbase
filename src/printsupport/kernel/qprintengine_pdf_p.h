/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
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
#include "QtGui/qmatrix.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qpainterpath.h"
#include "QtCore/qdatastream.h"

#include "private/qfontengine_p.h"
#include "private/qpdf_p.h"
#include "private/qpaintengine_p.h"
#include "qprintengine.h"

QT_BEGIN_NAMESPACE

class QImage;
class QDataStream;
class QPen;
class QPointF;
class QRegion;
class QFile;
class QPdfPrintEngine;

#define PPK_CupsOptions QPrintEngine::PrintEnginePropertyKey(0xfe00)
#define PPK_CupsPageRect QPrintEngine::PrintEnginePropertyKey(0xfe01)
#define PPK_CupsPaperRect QPrintEngine::PrintEnginePropertyKey(0xfe02)
#define PPK_CupsStringPageSize QPrintEngine::PrintEnginePropertyKey(0xfe03)

namespace QPdf {

    struct PaperSize {
        int width, height; // in postscript points
    };
    Q_PRINTSUPPORT_EXPORT PaperSize paperSize(QPrinter::PaperSize paperSize);
    Q_PRINTSUPPORT_EXPORT const char *paperSizeToString(QPrinter::PaperSize paperSize);
}

class QPdfPrintEnginePrivate;

class QPdfPrintEngine : public QPdfEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QPdfPrintEngine)
public:
    QPdfPrintEngine(QPrinter::PrinterMode m);
    virtual ~QPdfPrintEngine();

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev);
    bool end();
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() {return false;}
    QPrinter::PrinterState printerState() const {return state;}

    bool newPage();
    int metric(QPaintDevice::PaintDeviceMetric) const;
    void setProperty(PrintEnginePropertyKey key, const QVariant &value);
    QVariant property(PrintEnginePropertyKey key) const;
    // end reimplementations QPrintEngine

    QPrinter::PrinterState state;

private:
    Q_DISABLE_COPY(QPdfPrintEngine)
};

class QPdfPrintEnginePrivate : public QPdfEnginePrivate
{
    Q_DECLARE_PUBLIC(QPdfPrintEngine)
public:
    QPdfPrintEnginePrivate(QPrinter::PrinterMode m);
    ~QPdfPrintEnginePrivate();

    bool openPrintDevice();
    void closePrintDevice();

    void updatePaperSize();

private:
    Q_DISABLE_COPY(QPdfPrintEnginePrivate)

    QString printerName;
    QString printProgram;
    QString selectionOption;
    QStringList cupsOptions;
    QString cupsStringPageSize;

    QPrinter::DuplexMode duplex;
    bool collate;
    int copies;
    QPrinter::PageOrder pageOrder;
    QPrinter::PaperSource paperSource;

    QPrinter::PaperSize printerPaperSize;
    QRect cupsPaperRect;
    QRect cupsPageRect;
    QSizeF customPaperSize; // in postscript points

    int fd;
#if !defined(QT_NO_CUPS)
    QString cupsTempFile;
#endif // QT_NO_CUPS
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_PDF_P_H
