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

#include <QtPrintSupport/qprintengine.h>

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "private/qcups_p.h"
#include "qprinterinfo.h"

#ifndef QT_NO_PRINTER
#include <limits.h>
#include <math.h>

#include "qprintengine_pdf_p.h"

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
    Q_D(QPdfPrintEngine);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        const cups_dest_t* printers = cups.availablePrinters();
        int prnCount = cups.availablePrintersCount();

        for (int i = 0; i <  prnCount; ++i) {
            if (printers[i].is_default) {
                d->printerName = QString::fromLocal8Bit(printers[i].name);
                break;
            }
        }

    } else
#endif
    {
        d->printerName = QString::fromLocal8Bit(qgetenv("PRINTER"));
        if (d->printerName.isEmpty())
            d->printerName = QString::fromLocal8Bit(qgetenv("LPDEST"));
        if (d->printerName.isEmpty())
            d->printerName = QString::fromLocal8Bit(qgetenv("NPRINTER"));
        if (d->printerName.isEmpty())
            d->printerName = QString::fromLocal8Bit(qgetenv("NGPRINTER"));
    }

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
        d->cupsStringPageSize = value.toString();
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
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
        if (QCUPSSupport::isAvailable())
            ret = true;
        else
#endif
            ret = false;
        break;
    case PPK_NumberOfCopies:
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
        if (QCUPSSupport::isAvailable())
            ret = 1;
        else
#endif
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
        ret = d->cupsStringPageSize;
        break;
    case PPK_CustomPaperSize:
        ret = d->customPaperSize;
        break;
    case PPK_PageMargins:
    {
        QList<QVariant> margins;
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


#ifndef QT_NO_LPR
static void closeAllOpenFds()
{
    // hack time... getting the maximum number of open
    // files, if possible.  if not we assume it's the
    // larger of 256 and the fd we got
    int i;
#if defined(_SC_OPEN_MAX)
    i = (int)sysconf(_SC_OPEN_MAX);
#elif defined(_POSIX_OPEN_MAX)
    i = (int)_POSIX_OPEN_MAX;
#elif defined(OPEN_MAX)
    i = (int)OPEN_MAX;
#else
    i = 256;
#endif
    // leave stdin/out/err untouched
    while(--i > 2)
        QT_CLOSE(i);
}
#endif

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
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
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
#endif
#ifndef QT_NO_LPR
    } else {
        QString pr;
        if (!printerName.isEmpty())
            pr = printerName;
        int fds[2];
        if (qt_safe_pipe(fds) != 0) {
            qWarning("QPdfPrinter: Could not open pipe to print");
            return false;
        }

        pid_t pid = fork();
        if (pid == 0) {       // child process
            // if possible, exit quickly, so the actual lp/lpr
            // becomes a child of init, and ::waitpid() is
            // guaranteed not to wait.
            if (fork() > 0) {
                closeAllOpenFds();

                // try to replace this process with "true" - this prevents
                // global destructors from being called (that could possibly
                // do wrong things to the parent process)
                (void)execlp("true", "true", (char *)0);
                (void)execl("/bin/true", "true", (char *)0);
                (void)execl("/usr/bin/true", "true", (char *)0);
                ::_exit(0);
            }
            qt_safe_dup2(fds[0], 0, 0);

            closeAllOpenFds();

            if (!printProgram.isEmpty()) {
                if (!selectionOption.isEmpty())
                    pr.prepend(selectionOption);
                else
                    pr.prepend(QLatin1String("-P"));
                (void)execlp(printProgram.toLocal8Bit().data(), printProgram.toLocal8Bit().data(),
                             pr.toLocal8Bit().data(), (char *)0);
            } else {
                // if no print program has been specified, be smart
                // about the option string too.
                QList<QByteArray> lprhack;
                QList<QByteArray> lphack;
                QByteArray media;
                if (!pr.isEmpty() || !selectionOption.isEmpty()) {
                    if (!selectionOption.isEmpty()) {
                        QStringList list = selectionOption.split(QLatin1Char(' '));
                        for (int i = 0; i < list.size(); ++i)
                            lprhack.append(list.at(i).toLocal8Bit());
                        lphack = lprhack;
                    } else {
                        lprhack.append("-P");
                        lphack.append("-d");
                    }
                    lprhack.append(pr.toLocal8Bit());
                    lphack.append(pr.toLocal8Bit());
                }
                lphack.append("-s");

                char ** lpargs = new char *[lphack.size()+6];
                char lp[] = "lp";
                lpargs[0] = lp;
                int i;
                for (i = 0; i < lphack.size(); ++i)
                    lpargs[i+1] = (char *)lphack.at(i).constData();
#ifndef Q_OS_OSF
                if (QPdf::paperSizeToString(printerPaperSize)) {
                    char dash_o[] = "-o";
                    lpargs[++i] = dash_o;
                    lpargs[++i] = const_cast<char *>(QPdf::paperSizeToString(printerPaperSize));
                    lpargs[++i] = dash_o;
                    media = "media=";
                    media += QPdf::paperSizeToString(printerPaperSize);
                    lpargs[++i] = media.data();
                }
#endif
                lpargs[++i] = 0;
                char **lprargs = new char *[lprhack.size()+2];
                char lpr[] = "lpr";
                lprargs[0] = lpr;
                for (int i = 0; i < lprhack.size(); ++i)
                    lprargs[i+1] = (char *)lprhack[i].constData();
                lprargs[lprhack.size() + 1] = 0;
                (void)execvp("lp", lpargs);
                (void)execvp("lpr", lprargs);
                (void)execv("/bin/lp", lpargs);
                (void)execv("/bin/lpr", lprargs);
                (void)execv("/usr/bin/lp", lpargs);
                (void)execv("/usr/bin/lpr", lprargs);

                delete []lpargs;
                delete []lprargs;
            }
            // if we couldn't exec anything, close the fd,
            // wait for a second so the parent process (the
            // child of the GUI process) has exited.  then
            // exit.
            QT_CLOSE(0);
            (void)::sleep(1);
            ::_exit(0);
        }
        // parent process
        QT_CLOSE(fds[0]);
        fd = fds[1];
        (void)qt_safe_waitpid(pid, 0, 0);

        if (fd < 0)
            return false;

        outDevice = new QFile();
        static_cast<QFile *>(outDevice)->open(fd, QIODevice::WriteOnly);
#endif
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

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
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

        if (!cupsStringPageSize.isEmpty()) {
            options.append(QPair<QByteArray, QByteArray>("media", cupsStringPageSize.toLocal8Bit()));
        }

        if (copies > 1) {
            options.append(QPair<QByteArray, QByteArray>("copies", QString::number(copies).toLocal8Bit()));
        }

        if (collate) {
            options.append(QPair<QByteArray, QByteArray>("Collate", "True"));
        }

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

        if (QCUPSSupport::cupsVersion() >= 10300 && landscape) {
            options.append(QPair<QByteArray, QByteArray>("landscape", ""));
        }

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
#endif
}



QPdfPrintEnginePrivate::QPdfPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfEnginePrivate(),
      duplex(QPrinter::DuplexNone),
      collate(false),
      copies(1),
      pageOrder(QPrinter::FirstPageFirst),
      paperSource(QPrinter::Auto),
      printerPaperSize(QPrinter::A4),
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
    } else if (!cupsPaperRect.isNull()) {
        QRect r = cupsPaperRect;
        paperSize = r.size();
    } else{
        QPdf::PaperSize s = QPdf::paperSize(printerPaperSize);
        paperSize = QSize(s.width, s.height);
    }
}


QT_END_NAMESPACE

#endif // QT_NO_PRINTER
