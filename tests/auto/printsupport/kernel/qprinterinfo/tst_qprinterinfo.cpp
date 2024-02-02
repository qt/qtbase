// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtPrintSupport/qprinterinfo.h>

#include <algorithm>

#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_VXWORKS)
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  define USE_PIPE_EXEC
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

#ifdef USE_PIPE_EXEC
    QString getOutputFromCommand(const QStringList& command);
#endif // USE_PIPE_EXEC
#endif
};


#ifdef QT_NO_PRINTER
void tst_QPrinterInfo::initTestCase()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() == 33)
        QSKIP("Is flaky on Android 13 / RHEL 8.6 and 8.8 (QTQAINFRA-5606)");
#endif
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
#ifdef USE_PIPE_EXEC
    QStringList command;
    command << "lpstat" << "-d";
    QString output = getOutputFromCommand(command);

    QRegularExpression noDefaultReg("[^:]*no .*default");
    QRegularExpressionMatch match;
    match = noDefaultReg.match(output);
    if (match.hasMatch())
        return QString();

    QRegularExpression defaultReg("default.*: *([a-zA-Z0-9_-]+)");
    match = defaultReg.match(output);
    printer = match.captured(1);
#endif // USE_PIPE_EXEC
    return printer;
}

QStringList tst_QPrinterInfo::getPrintersFromSystem()
{
    QStringList ans;

#ifdef Q_OS_WIN32
    // TODO "cscript c:\windows\system32\prnmngr.vbs -l"
#endif // Q_OS_WIN32
#ifdef USE_PIPE_EXEC
    QString output = getOutputFromCommand({ "lpstat", "-e" });
    QStringList list = output.split(QChar::fromLatin1('\n'));

    QRegularExpression reg("^([.a-zA-Z0-9-_@/]+)");
    QRegularExpressionMatch match;
    for (int c = 0; c < list.size(); ++c) {
        match = reg.match(list[c]);
        if (match.hasMatch()) {
            QString printer = match.captured(1);
            ans << printer;
        }
    }
#endif // USE_PIPE_EXEC

    return ans;
}

#ifdef USE_PIPE_EXEC
// This function does roughly the same as the `command substitution` in
// the shell.
QString getOutputFromCommandInternal(const QStringList &command)
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

QString tst_QPrinterInfo::getOutputFromCommand(const QStringList &command)
{
    // Forces the ouptut from the command to be in English
    const QByteArray origSoftwareEnv = qgetenv("SOFTWARE");
    qputenv("SOFTWARE", nullptr);
    QString output = getOutputFromCommandInternal(command);
    qputenv("SOFTWARE", origSoftwareEnv);
    return output;
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
        QCOMPARE(copy1.supportedPageSizes(),      printers.at(i).supportedPageSizes());
        QCOMPARE(copy1.supportedResolutions(),    printers.at(i).supportedResolutions());
        QCOMPARE(copy1.defaultDuplexMode(),       printers.at(i).defaultDuplexMode());
        QCOMPARE(copy1.supportedDuplexModes(),    printers.at(i).supportedDuplexModes());

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
        QCOMPARE(copy2.supportedPageSizes(),      printers.at(i).supportedPageSizes());
        QCOMPARE(copy2.supportedResolutions(),    printers.at(i).supportedResolutions());
        QCOMPARE(copy2.defaultDuplexMode(),       printers.at(i).defaultDuplexMode());
        QCOMPARE(copy2.supportedDuplexModes(),    printers.at(i).supportedDuplexModes());
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
        QCOMPARE(copy.defaultDuplexMode(),       printers.at(i).defaultDuplexMode());
        QCOMPARE(copy.supportedDuplexModes(),    printers.at(i).supportedDuplexModes());
    }
}

void tst_QPrinterInfo::namedPrinter()
{
    const QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    QStringList printerNames;

    for (const QPrinterInfo &pi : printers) {
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
        QCOMPARE(pi2.defaultDuplexMode(),       pi.defaultDuplexMode());
        QCOMPARE(pi2.supportedDuplexModes(),    pi.supportedDuplexModes());
    }
}
#endif // QT_NO_PRINTER

QTEST_MAIN(tst_QPrinterInfo)
#include "tst_qprinterinfo.moc"
