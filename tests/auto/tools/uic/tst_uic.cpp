/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QLibraryInfo>

class tst_uic : public QObject
{
    Q_OBJECT

public:
    tst_uic();

private Q_SLOTS:
    void initTestCase();

    void run();
    void run_data() const;

    void compare();
    void compare_data() const;

    void cleanupTestCase();

private:
    QString workingDir() const;

    const QString command;
};


tst_uic::tst_uic()
    : command(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/uic"))
{
}

void tst_uic::initTestCase()
{
    QProcess process;
    process.start(command, QStringList(QLatin1String("-help")));

    if (!process.waitForFinished()) {
        const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
        QString message = QString::fromLatin1("'%1' could not be found when run from '%2'. Path: '%3' ").
                          arg(command, QDir::currentPath(), path);
        QFAIL(qPrintable(message));
    }
    // Print version
    const QString out = QString::fromLocal8Bit(process.readAllStandardError()).remove(QLatin1Char('\r'));
    const QStringList outLines = out.split(QLatin1Char('\n'));
    // Print version
    QString msg = QString::fromLatin1("uic test built %1 running in '%2' using: ").
                  arg(QString::fromLatin1(__DATE__), QDir::currentPath());
    if (!outLines.empty())
        msg += outLines.front();
    qDebug() << msg;
    process.terminate();

    QCOMPARE(QFileInfo(QLatin1String(SRCDIR "baseline")).exists(), true);
    QCOMPARE(QFileInfo(QLatin1String(SRCDIR "generated_ui")).exists(), true);
}

void tst_uic::run()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);

    QProcess process;
    process.setWorkingDirectory(workingDir());

    process.start(command, QStringList(originalFile) << QString(QLatin1String("-o"))
        << generatedFile);

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    if (process.waitForFinished()) {
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(QFileInfo(generatedFile).exists(), true);
    } else {
        QString error(QLatin1String("could not generated file: "));
        QFAIL(error.append(generatedFile).toUtf8().constData());
    }
}

void tst_uic::run_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    QString cwd = workingDir().append(QDir::separator());

    QDir dir(cwd + QLatin1String("baseline"));
    QFileInfoList originalFiles = dir.entryInfoList(QStringList("*.ui"), QDir::Files);

    dir.setPath(cwd + QLatin1String("generated_ui"));
    for (int i = 0; i < originalFiles.count(); ++i) {
        QTest::newRow(qPrintable(originalFiles.at(i).baseName()))
            << originalFiles.at(i).absoluteFilePath()
            << dir.absolutePath() + QDir::separator()
                + originalFiles.at(i).fileName().append(QLatin1String(".h"));
    }
}


void tst_uic::compare()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);

    QFile orgFile(originalFile);
    QFile genFile(generatedFile);

    if (!orgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(orgFile.fileName()).toUtf8());
    }

    if (!genFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(genFile.fileName()).toUtf8());
    }

    originalFile = orgFile.readAll();
    originalFile.replace(QRegExp(QLatin1String("Created:.{0,25}[\\d]{4,4}")), "");
    originalFile.replace(QRegExp(QLatin1String("by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    generatedFile = genFile.readAll();
    generatedFile.replace(QRegExp(QLatin1String("Created:.{0,25}[\\d]{4,4}")), "");
    generatedFile.replace(QRegExp(QLatin1String("by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    QCOMPARE(generatedFile, originalFile);
}

void tst_uic::compare_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    QString cwd = workingDir().append(QDir::separator());

    QDir dir(cwd + QLatin1String("baseline"));
    QFileInfoList originalFiles = dir.entryInfoList(QStringList("*.h"), QDir::Files);

    dir.setPath(cwd + QLatin1String("generated_ui"));
    for (int i = 0; i < originalFiles.count(); ++i) {
        QTest::newRow(qPrintable(originalFiles.at(i).baseName()))
            << originalFiles.at(i).absoluteFilePath()
            << dir.absolutePath() + QDir::separator()
                + originalFiles.at(i).fileName();
    }
}

void tst_uic::cleanupTestCase()
{
    QString cwd = workingDir().append(QDir::separator());
    QDir dir(cwd.append(QLatin1String("/generated_ui")));
    QFileInfoList generatedFiles = dir.entryInfoList(QDir::Files);

    foreach (const QFileInfo& fInfo, generatedFiles) {
        QFile file(fInfo.absoluteFilePath());
  //      file.remove();
    }
}

QString tst_uic::workingDir() const
{
    return QDir::cleanPath(SRCDIR);
}

QTEST_MAIN(tst_uic)
#include "tst_uic.moc"
