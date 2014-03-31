/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtPrintSupport/qprinterinfo.h>

#include <algorithm>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif


class tst_QPrinterInfo : public QObject
{
    Q_OBJECT

public slots:
#ifdef QT_NO_PRINTER
    void initTestCase();
    void cleanupTestCase();
#else
private slots:
#ifndef Q_OS_WIN32
    void testForDefaultPrinter();
    void testForPrinters();
#endif
    void testForPaperSizes();
    void testConstructors();
    void testAssignment();
    void namedPrinter();

private:
    QString getDefaultPrinterFromSystem();
    QStringList getPrintersFromSystem();

#ifdef Q_OS_UNIX
    QString getOutputFromCommand(const QStringList& command);
#endif // Q_OS_UNIX
#endif
};


#ifdef QT_NO_PRINTER
void tst_QPrinterInfo::initTestCase()
{
    QSKIP("This test requires printing support");
}

void tst_QPrinterInfo::cleanupTestCase()
{
    QSKIP("This test requires printing support");
}

#else
QString tst_QPrinterInfo::getDefaultPrinterFromSystem()
{
    QString printer;

#ifdef Q_OS_WIN32
    // TODO "cscript c:\windows\system32\prnmngr.vbs -g"
#endif // Q_OS_WIN32
#ifdef Q_OS_UNIX
    QStringList command;
    command << "lpstat" << "-d";
    QString output = getOutputFromCommand(command);

    QRegExp noDefaultReg("[^:]*no .*default");
    int pos = noDefaultReg.indexIn(output);
    if (pos >= 0) {
        return QString();
    }

    QRegExp defaultReg("default.*: *([a-zA-Z0-9_-]+)");
    defaultReg.indexIn(output);
    printer = defaultReg.cap(1);
#endif // Q_OS_UNIX

    return printer;
}

QStringList tst_QPrinterInfo::getPrintersFromSystem()
{
    QStringList ans;

#ifdef Q_OS_WIN32
    // TODO "cscript c:\windows\system32\prnmngr.vbs -l"
#endif // Q_OS_WIN32
#ifdef Q_OS_UNIX
    QStringList command;
    command << "lpstat" << "-p";
    QString output = getOutputFromCommand(command);
    QStringList list = output.split(QChar::fromLatin1('\n'));

    QRegExp reg("^[Pp]rinter ([.a-zA-Z0-9-_@]+)");
    for (int c = 0; c < list.size(); ++c) {
        if (reg.indexIn(list[c]) >= 0) {
            QString printer = reg.cap(1);
            ans << printer;
        }
    }
#endif // Q_OS_UNIX

    return ans;
}

#ifdef Q_OS_UNIX
// This function does roughly the same as the `command substitution` in
// the shell.
QString tst_QPrinterInfo::getOutputFromCommand(const QStringList& command)
{
// The command execution does nothing on non-unix systems.
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
}
#endif

// Windows test support not yet implemented
#ifndef Q_OS_WIN32
void tst_QPrinterInfo::testForDefaultPrinter()
{
    QString testPrinter = getDefaultPrinterFromSystem();
    QString defaultPrinter = QPrinterInfo::defaultPrinter().printerName();
    QString availablePrinter;
    int availablePrinterDefaults = 0;

    QList<QPrinterInfo> list = QPrinterInfo::availablePrinters();
    for (int c = 0; c < list.size(); ++c) {
        if (list[c].isDefault()) {
            availablePrinter = list.at(c).printerName();
            ++availablePrinterDefaults;
        }
    }

    qDebug() << "Test believes Default Printer                              = " << testPrinter;
    qDebug() << "QPrinterInfo::defaultPrinter() believes Default Printer    = " << defaultPrinter;
    qDebug() << "QPrinterInfo::availablePrinters() believes Default Printer = " << availablePrinter;

    QCOMPARE(testPrinter, defaultPrinter);
    QCOMPARE(testPrinter, availablePrinter);
    if (!availablePrinter.isEmpty())
        QCOMPARE(availablePrinterDefaults, 1);
}
#endif

// Windows test support not yet implemented
#ifndef Q_OS_WIN32
void tst_QPrinterInfo::testForPrinters()
{
    QStringList testPrinters = getPrintersFromSystem();

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    QStringList qtPrinters;
    for (int i = 0; i < printers.size(); ++i)
        qtPrinters.append(printers.at(i).printerName());

    std::sort(testPrinters.begin(), testPrinters.end());
    std::sort(qtPrinters.begin(), qtPrinters.end());

    qDebug() << "Test believes Available Printers                              = " << testPrinters;
    qDebug() << "QPrinterInfo::availablePrinters() believes Available Printers = " << qtPrinters;

    QCOMPARE(qtPrinters.size(), testPrinters.size());

    for (int i = 0; i < testPrinters.size(); ++i)
        QCOMPARE(qtPrinters.at(i), testPrinters.at(i));
}
#endif

void tst_QPrinterInfo::testForPaperSizes()
{
    // TODO Old PaperSize test dependent on physical printer installed, new generic test required
    // In the meantime just exercise the code path and print-out for inspection.

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    for (int i = 0; i < printers.size(); ++i) {
        qDebug() << "Printer       : " << printers.at(i).printerName() << printers.at(i).defaultPageSize();
        qDebug() << "Paper Sizes   : " << printers.at(i).supportedPageSizes();
        qDebug() << "Custom Sizes  : " << printers.at(i).supportsCustomPageSizes();
        qDebug() << "Physical Sizes: " << printers.at(i).minimumPhysicalPageSize()
                                       << printers.at(i).maximumPhysicalPageSize();
        qDebug() << "";
    }
}

void tst_QPrinterInfo::testConstructors()
{
    QPrinterInfo null;
    QCOMPARE(null.printerName(), QString());
    QVERIFY(null.isNull());

    QPrinterInfo null2(null);
    QVERIFY(null2.isNull());

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    for (int i = 0; i < printers.size(); ++i) {
        QPrinterInfo copy1(printers.at(i));
        QCOMPARE(copy1.printerName(),             printers.at(i).printerName());
        QCOMPARE(copy1.description(),             printers.at(i).description());
        QCOMPARE(copy1.location(),                printers.at(i).location());
        QCOMPARE(copy1.makeAndModel(),            printers.at(i).makeAndModel());
        QCOMPARE(copy1.isNull(),                  printers.at(i).isNull());
        QCOMPARE(copy1.isDefault(),               printers.at(i).isDefault());
        QCOMPARE(copy1.isRemote(),                printers.at(i).isRemote());
        QCOMPARE(copy1.state(),                   printers.at(i).state());
        QCOMPARE(copy1.supportedPageSizes(),      printers.at(i).supportedPageSizes());
        QCOMPARE(copy1.defaultPageSize(),         printers.at(i).defaultPageSize());
        QCOMPARE(copy1.supportsCustomPageSizes(), printers.at(i).supportsCustomPageSizes());
        QCOMPARE(copy1.minimumPhysicalPageSize(), printers.at(i).minimumPhysicalPageSize());
        QCOMPARE(copy1.maximumPhysicalPageSize(), printers.at(i).maximumPhysicalPageSize());
        QCOMPARE(copy1.supportedPaperSizes(),     printers.at(i).supportedPaperSizes());
        QCOMPARE(copy1.supportedSizesWithNames(), printers.at(i).supportedSizesWithNames());
        QCOMPARE(copy1.supportedResolutions(),    printers.at(i).supportedResolutions());

        QPrinter printer(printers.at(i));
        QPrinterInfo copy2(printer);
        QCOMPARE(copy2.printerName(),             printers.at(i).printerName());
        QCOMPARE(copy2.description(),             printers.at(i).description());
        QCOMPARE(copy2.location(),                printers.at(i).location());
        QCOMPARE(copy2.makeAndModel(),            printers.at(i).makeAndModel());
        QCOMPARE(copy2.isNull(),                  printers.at(i).isNull());
        QCOMPARE(copy2.isDefault(),               printers.at(i).isDefault());
        QCOMPARE(copy2.isRemote(),                printers.at(i).isRemote());
        QCOMPARE(copy2.state(),                   printers.at(i).state());
        QCOMPARE(copy2.supportedPageSizes(),      printers.at(i).supportedPageSizes());
        QCOMPARE(copy2.defaultPageSize(),         printers.at(i).defaultPageSize());
        QCOMPARE(copy2.supportsCustomPageSizes(), printers.at(i).supportsCustomPageSizes());
        QCOMPARE(copy2.minimumPhysicalPageSize(), printers.at(i).minimumPhysicalPageSize());
        QCOMPARE(copy2.maximumPhysicalPageSize(), printers.at(i).maximumPhysicalPageSize());
        QCOMPARE(copy2.supportedPaperSizes(),     printers.at(i).supportedPaperSizes());
        QCOMPARE(copy2.supportedSizesWithNames(), printers.at(i).supportedSizesWithNames());
        QCOMPARE(copy2.supportedResolutions(),    printers.at(i).supportedResolutions());
    }
}

void tst_QPrinterInfo::testAssignment()
{
    QPrinterInfo null;
    QVERIFY(null.isNull());
    QPrinterInfo null2;
    null2 = null;
    QVERIFY(null2.isNull());

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    for (int i = 0; i < printers.size(); ++i) {
        QPrinterInfo copy;
        copy = printers.at(i);
        QCOMPARE(copy.printerName(),             printers.at(i).printerName());
        QCOMPARE(copy.description(),             printers.at(i).description());
        QCOMPARE(copy.location(),                printers.at(i).location());
        QCOMPARE(copy.makeAndModel(),            printers.at(i).makeAndModel());
        QCOMPARE(copy.isNull(),                  printers.at(i).isNull());
        QCOMPARE(copy.isDefault(),               printers.at(i).isDefault());
        QCOMPARE(copy.isRemote(),                printers.at(i).isRemote());
        QCOMPARE(copy.state(),                   printers.at(i).state());
        QCOMPARE(copy.supportedPageSizes(),      printers.at(i).supportedPageSizes());
        QCOMPARE(copy.defaultPageSize(),         printers.at(i).defaultPageSize());
        QCOMPARE(copy.supportsCustomPageSizes(), printers.at(i).supportsCustomPageSizes());
        QCOMPARE(copy.minimumPhysicalPageSize(), printers.at(i).minimumPhysicalPageSize());
        QCOMPARE(copy.maximumPhysicalPageSize(), printers.at(i).maximumPhysicalPageSize());
        QCOMPARE(copy.supportedResolutions(),    printers.at(i).supportedResolutions());
    }
}

void tst_QPrinterInfo::namedPrinter()
{
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    QStringList printerNames;

    foreach (const QPrinterInfo &pi, printers) {
        QPrinterInfo pi2 = QPrinterInfo::printerInfo(pi.printerName());
        QCOMPARE(pi2.printerName(),             pi.printerName());
        QCOMPARE(pi2.description(),             pi.description());
        QCOMPARE(pi2.location(),                pi.location());
        QCOMPARE(pi2.makeAndModel(),            pi.makeAndModel());
        QCOMPARE(pi2.isNull(),                  pi.isNull());
        QCOMPARE(pi2.isDefault(),               pi.isDefault());
        QCOMPARE(pi2.isRemote(),                pi.isRemote());
        QCOMPARE(pi2.supportedPageSizes(),      pi.supportedPageSizes());
        QCOMPARE(pi2.defaultPageSize(),         pi.defaultPageSize());
        QCOMPARE(pi2.supportsCustomPageSizes(), pi.supportsCustomPageSizes());
        QCOMPARE(pi2.minimumPhysicalPageSize(), pi.minimumPhysicalPageSize());
        QCOMPARE(pi2.maximumPhysicalPageSize(), pi.maximumPhysicalPageSize());
        QCOMPARE(pi2.supportedResolutions(),    pi.supportedResolutions());
    }
}
#endif // QT_NO_PRINTER

QTEST_MAIN(tst_QPrinterInfo)
#include "tst_qprinterinfo.moc"
