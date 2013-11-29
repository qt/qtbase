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

#include "qcupsprintengine_p.h"

#ifndef QT_NO_PRINTER

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "private/qcups_p.h"
#include "qprinterinfo.h"

#include <limits.h>
#include <math.h>

#include "private/qcore_unix_p.h" // overrides QT_OPEN

QT_BEGIN_NAMESPACE

QCupsPrintEngine::QCupsPrintEngine(QPrinter::PrinterMode m)
    : QPdfPrintEngine(*new QCupsPrintEnginePrivate(m))
{
    Q_D(QCupsPrintEngine);

    if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        const cups_dest_t* printers = cups.availablePrinters();
        int prnCount = cups.availablePrintersCount();

        for (int i = 0; i <  prnCount; ++i) {
            if (printers[i].is_default) {
                d->printerName = QString::fromLocal8Bit(printers[i].name);
                d->setCupsDefaults();
                break;
            }
        }

    }

    state = QPrinter::Idle;
}

QCupsPrintEngine::~QCupsPrintEngine()
{
}

void QCupsPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QCupsPrintEngine);

    switch (int(key)) {
    case PPK_PaperSize:
        d->printerPaperSize = QPrinter::PaperSize(value.toInt());
        d->setPaperSize();
        break;
    case PPK_CupsPageRect:
        d->cupsPageRect = value.toRect();
        break;
    case PPK_CupsPaperRect:
        d->cupsPaperRect = value.toRect();
        break;
    case PPK_CupsOptions:
        d->cupsOptions = value.toStringList();
        break;
    case PPK_CupsStringPageSize:
    case PPK_PaperName:
        d->cupsStringPageSize = value.toString();
        d->setPaperName();
        break;
    case PPK_PrinterName:
        // prevent setting the defaults again for the same printer
        if (d->printerName != value.toString()) {
            d->printerName = value.toString();
            d->setCupsDefaults();
        }
        break;
    default:
        QPdfPrintEngine::setProperty(key, value);
        break;
    }
}

QVariant QCupsPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QCupsPrintEngine);

    QVariant ret;
    switch (int(key)) {
    case PPK_SupportsMultipleCopies:
        ret = true;
        break;
    case PPK_NumberOfCopies:
        ret = 1;
        break;
    case PPK_CupsPageRect:
        ret = d->cupsPageRect;
        break;
    case PPK_CupsPaperRect:
        ret = d->cupsPaperRect;
        break;
    case PPK_CupsOptions:
        ret = d->cupsOptions;
        break;
    case PPK_CupsStringPageSize:
    case PPK_PaperName:
        ret = d->cupsStringPageSize;
        break;
    default:
        ret = QPdfPrintEngine::property(key);
        break;
    }
    return ret;
}


QCupsPrintEnginePrivate::QCupsPrintEnginePrivate(QPrinter::PrinterMode m) : QPdfPrintEnginePrivate(m)
{
}

QCupsPrintEnginePrivate::~QCupsPrintEnginePrivate()
{
}

bool QCupsPrintEnginePrivate::openPrintDevice()
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
    } else if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        QPair<int, QString> ret = cups.tempFd();
        if (ret.first < 0) {
            qWarning("QPdfPrinter: Could not open temporary file to print");
            return false;
        }
        cupsTempFile = ret.second;
        outDevice = new QFile();
        static_cast<QFile *>(outDevice)->open(ret.first, QIODevice::WriteOnly);
        fd = ret.first;
    }

    return true;
}

void QCupsPrintEnginePrivate::closePrintDevice()
{
    QPdfPrintEnginePrivate::closePrintDevice();

    if (!cupsTempFile.isEmpty()) {
        QString tempFile = cupsTempFile;
        cupsTempFile.clear();
        QCUPSSupport cups;

        // Set up print options.
        QByteArray prnName;
        QList<QPair<QByteArray, QByteArray> > options;
        QVector<cups_option_t> cupsOptStruct;

        if (!printerName.isEmpty()) {
            prnName = printerName.toLocal8Bit();
        } else {
            QPrinterInfo def = QPrinterInfo::defaultPrinter();
            if (def.isNull()) {
                qWarning("Could not determine printer to print to");
                QFile::remove(tempFile);
                return;
            }
            prnName = def.printerName().toLocal8Bit();
        }

        if (!cupsStringPageSize.isEmpty())
            options.append(QPair<QByteArray, QByteArray>("media", cupsStringPageSize.toLocal8Bit()));

        if (copies > 1)
            options.append(QPair<QByteArray, QByteArray>("copies", QString::number(copies).toLocal8Bit()));

        if (copies > 1 && collate)
            options.append(QPair<QByteArray, QByteArray>("Collate", "True"));

        switch (duplex) {
        case QPrinter::DuplexNone:
            options.append(QPair<QByteArray, QByteArray>("sides", "one-sided"));
            break;
        case QPrinter::DuplexAuto:
            if (!landscape)
                options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-long-edge"));
            else
                options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-short-edge"));
            break;
        case QPrinter::DuplexLongSide:
            options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-long-edge"));
            break;
        case QPrinter::DuplexShortSide:
            options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-short-edge"));
            break;
        }

        if (QCUPSSupport::cupsVersion() >= 10300 && landscape)
            options.append(QPair<QByteArray, QByteArray>("landscape", ""));

        QStringList::const_iterator it = cupsOptions.constBegin();
        while (it != cupsOptions.constEnd()) {
            options.append(QPair<QByteArray, QByteArray>((*it).toLocal8Bit(), (*(it+1)).toLocal8Bit()));
            it += 2;
        }

        for (int c = 0; c < options.size(); ++c) {
            cups_option_t opt;
            opt.name = options[c].first.data();
            opt.value = options[c].second.data();
            cupsOptStruct.append(opt);
        }

        // Print the file.
        cups_option_t* optPtr = cupsOptStruct.size() ? &cupsOptStruct.first() : 0;
        cups.printFile(prnName.constData(), tempFile.toLocal8Bit().constData(),
                title.toLocal8Bit().constData(), cupsOptStruct.size(), optPtr);

        QFile::remove(tempFile);
    }
}

void QCupsPrintEnginePrivate::updatePaperSize()
{
    if (printerPaperSize == QPrinter::Custom) {
        paperSize = customPaperSize;
    } else if (!cupsPaperRect.isNull()) {
        QRect r = cupsPaperRect;
        paperSize = r.size();
    } else {
        QPdf::PaperSize s = QPdf::paperSize(printerPaperSize);
        paperSize = QSize(s.width, s.height);
    }
}

void QCupsPrintEnginePrivate::setPaperSize()
{
    if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        QPdf::PaperSize size = QPdf::paperSize(QPrinter::PaperSize(printerPaperSize));

        if (cups.currentPPD()) {
            cupsStringPageSize = QLatin1String("Custom");
            const ppd_option_t* pageSizes = cups.pageSizes();
            for (int i = 0; i < pageSizes->num_choices; ++i) {
                QByteArray cupsPageSize = pageSizes->choices[i].choice;
                QRect tmpCupsPaperRect = cups.paperRect(cupsPageSize);
                QRect tmpCupsPageRect = cups.pageRect(cupsPageSize);

                if (qAbs(size.width - tmpCupsPaperRect.width()) < 5  && qAbs(size.height - tmpCupsPaperRect.height()) < 5) {
                    cupsPaperRect = tmpCupsPaperRect;
                    cupsPageRect = tmpCupsPageRect;
                    cupsStringPageSize = pageSizes->choices[i].text;
                    leftMargin = cupsPageRect.x() - cupsPaperRect.x();
                    topMargin = cupsPageRect.y() - cupsPaperRect.y();
                    rightMargin = cupsPaperRect.right() - cupsPageRect.right();
                    bottomMargin = cupsPaperRect.bottom() - cupsPageRect.bottom();

                    updatePaperSize();
                    break;
                }
            }
        }
    }
}

void QCupsPrintEnginePrivate::setPaperName()
{
    if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        if (cups.currentPPD()) {
            const ppd_option_t* pageSizes = cups.pageSizes();
            bool foundPaperName = false;
            for (int i = 0; i < pageSizes->num_choices; ++i) {
                if (cupsStringPageSize == pageSizes->choices[i].text) {
                    foundPaperName = true;
                    QByteArray cupsPageSize = pageSizes->choices[i].choice;
                    cupsPaperRect = cups.paperRect(cupsPageSize);
                    cupsPageRect = cups.pageRect(cupsPageSize);
                    leftMargin = cupsPageRect.x() - cupsPaperRect.x();
                    topMargin = cupsPageRect.y() - cupsPaperRect.y();
                    rightMargin = cupsPaperRect.right() - cupsPageRect.right();
                    bottomMargin = cupsPaperRect.bottom() - cupsPageRect.bottom();
                    printerPaperSize = QPrinter::Custom;
                    customPaperSize = cupsPaperRect.size();
                    for (int ps = 0; ps < QPrinter::NPageSize; ++ps) {
                        QPdf::PaperSize size = QPdf::paperSize(QPrinter::PaperSize(ps));
                        if (qAbs(size.width - cupsPaperRect.width()) < 5 && qAbs(size.height - cupsPaperRect.height()) < 5) {
                            printerPaperSize = static_cast<QPrinter::PaperSize>(ps);
                            customPaperSize = QSize();
                            break;
                        }
                    }
                    updatePaperSize();
                    break;
                }
            }
            if (!foundPaperName)
                cupsStringPageSize = QString();
        }
    }
}

void QCupsPrintEnginePrivate::setCupsDefaults()
{
    if (QCUPSSupport::isAvailable()) {
        int cupsPrinterIndex = -1;
        QCUPSSupport cups;

        const cups_dest_t* printers = cups.availablePrinters();
        int prnCount = cups.availablePrintersCount();
        for (int i = 0; i <  prnCount; ++i) {
            QString name = QString::fromLocal8Bit(printers[i].name);
            if (name == printerName) {
                cupsPrinterIndex = i;
                break;
            }
        }

        if (cupsPrinterIndex < 0)
            return;

        cups.setCurrentPrinter(cupsPrinterIndex);

        if (cups.currentPPD()) {
            const ppd_option_t *ppdDuplex = cups.ppdOption("Duplex");
            if (ppdDuplex) {
                if (qstrcmp(ppdDuplex->defchoice, "DuplexTumble") == 0)
                    duplex = QPrinter::DuplexShortSide;
                else if (qstrcmp(ppdDuplex->defchoice, "DuplexNoTumble") == 0)
                    duplex = QPrinter::DuplexLongSide;
                else
                    duplex = QPrinter::DuplexNone;
            }

            grayscale = !cups.currentPPD()->color_device;

            const ppd_option_t *ppdCollate = cups.ppdOption("Collate");
            if (ppdCollate)
                collate = qstrcmp(ppdCollate->defchoice, "True") == 0;

            const ppd_option_t* pageSizes = cups.pageSizes();
            QByteArray cupsPageSize;
            for (int i = 0; i < pageSizes->num_choices; ++i) {
                if (static_cast<int>(pageSizes->choices[i].marked) == 1) {
                    cupsPageSize = pageSizes->choices[i].choice;
                    cupsStringPageSize = pageSizes->choices[i].text;
                }
            }

            cupsOptions = cups.options();
            cupsPaperRect = cups.paperRect(cupsPageSize);
            cupsPageRect = cups.pageRect(cupsPageSize);

            for (int ps = 0; ps < QPrinter::NPageSize; ++ps) {
                QPdf::PaperSize size = QPdf::paperSize(QPrinter::PaperSize(ps));
                if (qAbs(size.width - cupsPaperRect.width()) < 5 && qAbs(size.height - cupsPaperRect.height()) < 5) {
                    printerPaperSize = static_cast<QPrinter::PaperSize>(ps);

                    leftMargin = cupsPageRect.x() - cupsPaperRect.x();
                    topMargin = cupsPageRect.y() - cupsPaperRect.y();
                    rightMargin = cupsPaperRect.right() - cupsPageRect.right();
                    bottomMargin = cupsPaperRect.bottom() - cupsPageRect.bottom();

                    updatePaperSize();
                    break;
                }
            }
        }
    }
}


QT_END_NAMESPACE

#endif // QT_NO_PRINTER
