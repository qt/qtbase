/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtNetwork/QHostInfo>

#ifndef QT_NO_PRINTER
#include <qprinterinfo.h>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

Q_DECLARE_METATYPE(QRect)


#if defined(Q_OS_WIN32)
# define ACCEPTABLE_WINDOWS
#endif


//TESTED_CLASS=
//TESTED_FILES=

class tst_QPrinterInfo : public QObject
{
    Q_OBJECT

public:
    //tst_QPrinterInfo();
    //virtual ~tst_QPrinterInfo();


public slots:
    //void initTestCase();
    //void cleanupTestCase();
    //void init();
    //void cleanup();
private slots:
    void testForDefaultPrinter();
    void testForPrinters();
    void testForPaperSizes();
    void testConstructors();
    void testAssignment();

private:
    void macFixNameFormat(QString *printerName);
    QString getDefaultPrinterFromSystem();
    QStringList getPrintersFromSystem();

    QString getOutputFromCommand(const QStringList& command);
};

void tst_QPrinterInfo::macFixNameFormat(QString *printerName)
{
// Modify the format of the printer name to match Qt, lpstat returns
// foo___domain_no, Qt returns foo @ domain.no
#ifdef Q_WS_MAC
    printerName->replace(QLatin1String("___"), QLatin1String(" @ "));
    printerName->replace(QLatin1String("_"), QLatin1String("."));
#else
    Q_UNUSED(printerName);
#endif
}

QString tst_QPrinterInfo::getDefaultPrinterFromSystem()
{
    QStringList command;
    command << "lpstat" << "-d";
    QString output = getOutputFromCommand(command);

    QRegExp noDefaultReg("[^:]*no .*default");
    int pos = noDefaultReg.indexIn(output);
    if (pos >= 0) {
        return QString();
    }

    QRegExp defaultReg("default.*: *([a-zA-Z0-9_]+)");
    defaultReg.indexIn(output);
    QString printer = defaultReg.cap(1);
    macFixNameFormat(&printer);
    return printer;
}

QStringList tst_QPrinterInfo::getPrintersFromSystem()
{
    QStringList ans;

    QStringList command;
    command << "lpstat" << "-p";
    QString output = getOutputFromCommand(command);
    QStringList list = output.split(QChar::fromLatin1('\n'));

    QRegExp reg("^[Pp]rinter ([.a-zA-Z0-9-_@]+)");
    for (int c = 0; c < list.size(); ++c) {
        if (reg.indexIn(list[c]) >= 0) {
            QString printer = reg.cap(1);
            macFixNameFormat(&printer);
            ans << printer;
        }
    }

    return ans;
}

// This function does roughly the same as the `command substitution` in
// the shell.
QString tst_QPrinterInfo::getOutputFromCommand(const QStringList& command)
{
// The command execution does nothing on non-unix systems.
#ifdef Q_OS_UNIX
    int pid;
    int status = 0;
    int pipePtr[2];

    // Create a pipe that is shared between parent and child process.
    if (pipe(pipePtr) < 0) {
        return QString();
    }
    pid = fork();
    if (pid < 0) {
        close(pipePtr[0]);
        close(pipePtr[1]);
        return QString();
    } else if (pid == 0) {
        // In child.
        // Close the reading end.
        close(pipePtr[0]);
        // Redirect stdout to the pipe.
        if (dup2(pipePtr[1], 1) < 0) {
            exit(1);
        }

        char** argv = new char*[command.size()+1];
        for (int c = 0; c < command.size(); ++c) {
            argv[c] = new char[command[c].size()+1];
            strcpy(argv[c], command[c].toLatin1().data());
        }
        argv[command.size()] = NULL;
        execvp(argv[0], argv);
        // Shouldn't get here, but it's possible if command is not found.
        close(pipePtr[1]);
        close(1);
        for (int c = 0; c < command.size(); ++c) {
            delete [] argv[c];
        }
        delete [] argv;
        exit(1);
    } else {
        // In parent.
        // Close the writing end.
        close(pipePtr[1]);

        QFile pipeRead;
        if (!pipeRead.open(pipePtr[0], QIODevice::ReadOnly)) {
            close(pipePtr[0]);
            return QString();
        }
        QByteArray array;
        array = pipeRead.readAll();
        pipeRead.close();
        close(pipePtr[0]);
        wait(&status);
        return QString(array);
    }
#else
	return QString();
#endif // Q_OS_UNIX
}

void tst_QPrinterInfo::testForDefaultPrinter()
{
#if defined(Q_OS_UNIX) || defined(ACCEPTABLE_WINDOWS)
# ifdef ACCEPTABLE_WINDOWS
    if (QHostInfo::localHostName() == "fantomet" || QHostInfo::localHostName() == "bobo") {
        QWARN("Test is hardcoded to \"fantomet\" and \"bobo\" on Windows and may fail");
    } else {
        QSKIP("Test is hardcoded to \"fantomet\" and \"bobo\" on Windows", SkipAll);
    }
    QString defSysPrinter;
    if (QHostInfo::localHostName() == "fantomet") {
        defSysPrinter = "Yacc (Lexmark Optra T610 PS3)";
    } else if (QHostInfo::localHostName() == "bobo") {
        defSysPrinter = "press";
    }
# else
    QString defSysPrinter = getDefaultPrinterFromSystem();
# endif
    if (defSysPrinter == "") return;

    QList<QPrinterInfo> list = QPrinterInfo::availablePrinters();
    bool found = false;
    for (int c = 0; c < list.size(); ++c) {
        if (list[c].isDefault()) {
            QVERIFY(list.at(c).printerName() == defSysPrinter);
            QVERIFY(!list.at(c).isNull());
            found = true;
        } else {
            QVERIFY(list.at(c).printerName() != defSysPrinter);
            QVERIFY(!list.at(c).isNull());
        }
    }

    if (!found && defSysPrinter != "") QFAIL("No default printer reported by Qt, although there is one");
#else
    QSKIP("Test doesn't work on non-Unix", SkipAll);
#endif // defined(Q_OS_UNIX) || defined(ACCEPTABLE_WINDOWS)
}

void tst_QPrinterInfo::testForPrinters()
{
#if defined(Q_OS_UNIX) || defined(ACCEPTABLE_WINDOWS)
# ifdef ACCEPTABLE_WINDOWS
    if (QHostInfo::localHostName() == "fantomet" || QHostInfo::localHostName() == "bobo") {
        QWARN("Test is hardcoded to \"fantomet\" and \"bobo\" on Windows and may fail");
    } else {
        QSKIP("Test is hardcoded to \"fantomet\" and \"bobo\" on Windows", SkipAll);
    }
    QStringList sysPrinters;
    if (QHostInfo::localHostName() == "fantomet") {
        sysPrinters
                << "Press"
                << "Canon PS-IPU Color Laser Copier v52.3"
                << "EPSON EPL-N4000 PS3"
                << "Kroksleiven"
                << "Lexmark Optra Color 1200 PS"
                << "Yacc (Lexmark Optra T610 PCL)"
                << "Yacc (Lexmark Optra T610 PS3)"
                ;
    } else if (QHostInfo::localHostName() == "bobo") {
        sysPrinters
                << "press"
                << "finnmarka"
                << "nordmarka"
                ;
    }
# else
    QStringList sysPrinters = getPrintersFromSystem();
# endif
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    QCOMPARE(printers.size(), sysPrinters.size());

    QHash<QString, bool> qtPrinters;

    for (int j = 0; j < printers.size(); ++j) {
        qtPrinters.insert(printers.at(j).printerName(), !printers.at(j).isNull());
    }

    for (int i = 0; i < sysPrinters.size(); ++i) {
        if (!qtPrinters.value(sysPrinters.at(i))) {
            qDebug() << "Available printers: " << qtPrinters;
            QFAIL(qPrintable(QString("Printer '%1' reported by system, but not reported by Qt").arg(sysPrinters.at(i))));
        }
    }
#else
    QSKIP("Test doesn't work on non-Unix", SkipAll);
#endif // defined(Q_OS_UNIX) || defined(ACCEPTABLE_WINDOWS)
}

void tst_QPrinterInfo::testForPaperSizes()
{
QSKIP("PaperSize feature doesn't work on Windows, fails on Mac, and is unstable on Linux", SkipAll);
    // This test is based on common printers found at the Oslo
    // office. It is likely to be skipped or fail for other locations.
    QStringList hardPrinters;
    hardPrinters << "Finnmarka" << "Huldra";

    QList<QList<QPrinter::PaperSize> > hardSizes;
    hardSizes
            << QList<QPrinter::PaperSize>()
            << QList<QPrinter::PaperSize>()
            ;
    hardSizes[0] // Finnmarka
            << QPrinter::Letter
            << QPrinter::A4
            << QPrinter::A3
            << QPrinter::A5
            << QPrinter::B4
            << QPrinter::B5
            << QPrinter::Custom // COM10
            << QPrinter::Custom // C5
            << QPrinter::Custom // DL
            << QPrinter::Custom // Monarch
            << QPrinter::Executive
            << QPrinter::Custom // Foolscap
            << QPrinter::Custom // ISO B5
            << QPrinter::Ledger
            << QPrinter::Legal
            << QPrinter::Custom // Japanese Post Card
            << QPrinter::Custom // Invoice
            ;
    hardSizes[1] // Huldra
            << QPrinter::Custom // Not listed at http://localhost:631/, name "Custom"
            << QPrinter::Letter
            << QPrinter::A4
            << QPrinter::A5
            << QPrinter::A6
            << QPrinter::B5
            << QPrinter::Custom // #5 1/2 Envelope
            << QPrinter::Custom // 6x9 Envelope
            << QPrinter::Custom // #10 Envelope
            << QPrinter::Custom // A7 Envelope
            << QPrinter::Custom // C5 Envelope
            << QPrinter::Custom // DL Envelope
            << QPrinter::Custom // Monarch Envelope
            << QPrinter::Custom // #6 3/4 Envelope
            << QPrinter::Executive
            << QPrinter::Custom // US Folio
            << QPrinter::Custom // Index Card
            << QPrinter::Custom // ISO B5
            << QPrinter::Legal
            << QPrinter::Custom // Statement
            ;

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    for (int i = 0; i < printers.size(); ++i) {
        for (int j = 0; j < hardPrinters.size(); ++j) {
            if (printers[i].printerName() == hardPrinters[j]) {
                QList<QPrinter::PaperSize> sizes = printers[i].supportedPaperSizes();
                qSort(sizes);
                qSort(hardSizes[j]);
                QCOMPARE(sizes, hardSizes[j]);
            }
        }
    }
}

void tst_QPrinterInfo::testConstructors()
{
    QList<QPrinterInfo> prns(QPrinterInfo::availablePrinters());

    for (int c = 0; c < prns.size(); ++c) {
        QList<QPrinter::PaperSize> list1, list2;
        list1 = prns[c].supportedPaperSizes();
        QPrinter pr(prns[c]);
        list2 = QPrinterInfo(pr).supportedPaperSizes();
        QCOMPARE(list2, list1);
    }
}

void tst_QPrinterInfo::testAssignment()
{
    QList<QPrinterInfo> prns(QPrinterInfo::availablePrinters());

    for (int c = 0; c < prns.size(); ++c) {
        QPrinterInfo pi = QPrinterInfo::defaultPrinter();
        pi = prns[c];
        QCOMPARE(pi.printerName(), prns[c].printerName());
        QCOMPARE(pi.supportedPaperSizes(), prns[c].supportedPaperSizes());
    }
}

QTEST_MAIN(tst_QPrinterInfo)
#include "tst_qprinterinfo.moc"
#else
QTEST_NOOP_MAIN
#endif
