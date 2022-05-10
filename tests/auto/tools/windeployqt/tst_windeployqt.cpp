// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtTest/QtTest>

static const QString msgProcessError(const QProcess &process, const QString &what,
                                     const QByteArray &stdOut = QByteArray(),
                                     const QByteArray &stdErr = QByteArray())
{
    QString result;
    QTextStream str(&result);
    str << what << ": \"" << process.program() << ' '
        << process.arguments().join(QLatin1Char(' ')) << "\": " << process.errorString();
    if (!stdOut.isEmpty())
        str << "\nStandard output:\n" << stdOut;
    if (!stdErr.isEmpty())
        str << "\nStandard error:\n" << stdErr;
    return result;
}

static bool runProcess(const QString &binary,
                       const QStringList &arguments,
                       QString *errorMessage,
                       const QString &workingDir = QString(),
                       const QProcessEnvironment &env = QProcessEnvironment(),
                       int timeOut = 5000,
                       QByteArray *stdOutIn = nullptr, QByteArray *stdErrIn = nullptr)
{
    QProcess process;
    if (!env.isEmpty())
        process.setProcessEnvironment(env);
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);
    qDebug().noquote().nospace() << "Running: " << QDir::toNativeSeparators(binary)
        << ' ' << arguments.join(QLatin1Char(' '));
    process.start(binary, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        *errorMessage = msgProcessError(process, "Failed to start");
        return false;
    }
    if (!process.waitForFinished(timeOut)) {
        *errorMessage = msgProcessError(process, "Timed out");
        process.terminate();
        if (!process.waitForFinished(300))
            process.kill();
        return false;
    }
    const QByteArray stdOut = process.readAllStandardOutput();
    const QByteArray stdErr = process.readAllStandardError();
    if (stdOutIn)
        *stdOutIn = stdOut;
    if (stdErrIn)
        *stdErrIn = stdErr;
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = msgProcessError(process, "Crashed", stdOut, stdErr);
        return false;
    }
    if (process.exitCode() != QProcess::NormalExit) {
        *errorMessage = msgProcessError(process, "Exit code " + QString::number(process.exitCode()),
                                        stdOut, stdErr);
        return false;
    }
    return true;
}

class tst_windeployqt : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void help();
    void deploy();

private:
    QString m_windeployqtBinary;
    QString m_testApp;
    QString m_testAppBinary;
};

void tst_windeployqt::initTestCase()
{
    m_windeployqtBinary = QStandardPaths::findExecutable("windeployqt");
    QVERIFY(!m_windeployqtBinary.isEmpty());
    m_testApp = QFINDTESTDATA("testapp");
    QVERIFY(!m_testApp.isEmpty());
    const QFileInfo testAppBinary(m_testApp + QLatin1String("/testapp.exe"));
    QVERIFY2(testAppBinary.isFile(), qPrintable(testAppBinary.absoluteFilePath()));
    m_testAppBinary = testAppBinary.absoluteFilePath();
}

void tst_windeployqt::help()
{
    QString errorMessage;
    QByteArray stdOut;
    QByteArray stdErr;
    QVERIFY2(runProcess(m_windeployqtBinary, QStringList("--help"), &errorMessage,
                        QString(),  QProcessEnvironment(), 5000, &stdOut, &stdErr),
             qPrintable(errorMessage));
    QVERIFY2(!stdOut.isEmpty(), stdErr);
}

// deploy(): Deploys the test application and launches it with Qt removed from the environment
// to verify it runs stand-alone.

void tst_windeployqt::deploy()
{
    QString errorMessage;
    // Deploy application
    QStringList deployArguments;
    deployArguments << QLatin1String("--no-translations") << QDir::toNativeSeparators(m_testAppBinary);
    QVERIFY2(runProcess(m_windeployqtBinary, deployArguments, &errorMessage, QString(), QProcessEnvironment(), 20000),
             qPrintable(errorMessage));

    // Create environment with Qt and all "lib" paths removed.
    const QString qtBinDir = QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString pathKey = QLatin1String("PATH");
    const QChar pathSeparator(QLatin1Char(';')); // ### fixme: Qt 5.6: QDir::listSeparator()
    const QString origPath = env.value(pathKey);
    QString newPath;
    const QStringList pathElements = origPath.split(pathSeparator, Qt::SkipEmptyParts);
    for (const QString &pathElement : pathElements) {
        if (pathElement.compare(qtBinDir, Qt::CaseInsensitive)
            && !pathElement.contains(QLatin1String("\\lib"), Qt::CaseInsensitive)) {
            if (!newPath.isEmpty())
                newPath.append(pathSeparator);
            newPath.append(pathElement);
        }
    }
    if (newPath == origPath)
        qWarning() << "Unable to remove Qt from PATH";
    env.insert(pathKey, newPath);

    // Create qt.conf to enforce usage of local plugins
    QFile qtConf(QFileInfo(m_testAppBinary).absolutePath() + QLatin1String("/qt.conf"));
    QVERIFY2(qtConf.open(QIODevice::WriteOnly | QIODevice::Text),
             qPrintable(qtConf.fileName() + QLatin1String(": ") + qtConf.errorString()));
    QVERIFY(qtConf.write("[Paths]\nPrefix = .\n"));
    qtConf.close();

    // Verify that application still runs
    QVERIFY2(runProcess(m_testAppBinary, QStringList(), &errorMessage, QString(), env, 10000),
             qPrintable(errorMessage));
}

QTEST_MAIN(tst_windeployqt)
#include "tst_windeployqt.moc"
