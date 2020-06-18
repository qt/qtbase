/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QDirIterator>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QResource>
#include <QtCore/QLocale>
#include <QtCore/QtGlobal>

#include <algorithm>

typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap)

static QByteArray msgProcessStartFailed(const QProcess &p)
{
    const QString result = QLatin1String("Could not start \"")
        + QDir::toNativeSeparators(p.program()) + QLatin1String("\": ")
        + p.errorString();
    return result.toLocal8Bit();
}

static QByteArray msgProcessTimeout(const QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
        + "\" timed out.";
}

static QByteArray msgProcessCrashed(QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
        + "\" crashed.\n" + p.readAllStandardError();
}

static QByteArray msgProcessFailed(QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
        + "\" returned " + QByteArray::number(p.exitCode()) + ":\n"
        + p.readAllStandardError();
}

class tst_rcc : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void rcc_data();
    void rcc();

    void binary_data();
    void binary();

    void readback_data();
    void readback();

    void depFileGeneration_data();
    void depFileGeneration();

    void python();

    void cleanupTestCase();

private:
    QString m_rcc;
    QString m_dataPath;
};

void tst_rcc::initTestCase()
{
    // rcc uses a QHash to store files in the resource system.
    // we must force a certain hash order when testing or tst_rcc will fail, see QTBUG-25078
    QVERIFY(qputenv("QT_RCC_TEST", "1"));
    m_rcc = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/rcc");

    m_dataPath = QFINDTESTDATA("data");
    QVERIFY(!m_dataPath.isEmpty());
}


static inline bool isPythonComment(const QString &line)
{
    return line.startsWith(QLatin1Char('#'));
}

static QString doCompare(const QStringList &actual, const QStringList &expected,
                         const QString &timeStampPath)
{
    if (actual.size() != expected.size()) {
        return QString("Length count different: actual: %1, expected: %2")
            .arg(actual.size()).arg(expected.size());
    }

    QByteArray ba;
    const bool isPython = isPythonComment(expected.constFirst());
    for (int i = 0, n = expected.size(); i != n; ++i) {
        QString expectedLine = expected.at(i);
        if (expectedLine.startsWith("IGNORE:"))
            continue;
        if (isPython && isPythonComment(expectedLine) && isPythonComment(actual.at(i)))
            continue;
        if (expectedLine.startsWith("TIMESTAMP:")) {
            const QString relativePath = expectedLine.mid(strlen("TIMESTAMP:"));
            const QFileInfo fi(timeStampPath + QLatin1Char('/') + relativePath);
            if (!fi.isFile()) {
                ba.append("File " + fi.absoluteFilePath().toUtf8() + " does not exist!");
                break;
            }
            const quint64 timeStamp = quint64(fi.lastModified().toMSecsSinceEpoch());
            expectedLine.clear();
            for (int shift = 56; shift >= 0; shift -= 8) {
                expectedLine.append(QLatin1String("0x"));
                expectedLine.append(QString::number(quint8(timeStamp >> shift), 16));
                expectedLine.append(QLatin1Char(','));
            }
        }
        if (expectedLine != actual.at(i)) {
            qDebug() << "LINES" << (i + 1) << "DIFFER";
            ba.append(
             "\n<<<<<< actual\n" + actual.at(i).toUtf8() + "\n======\n" + expectedLine.toUtf8()
                + "\n>>>>>> expected\n"
            );
        }
    }
    return ba;
}

void tst_rcc::rcc_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("qrcfile");
    QTest::addColumn<QString>("expected");

    const QString imagesPath = m_dataPath + QLatin1String("/images");
    QTest::newRow("images") << imagesPath << "images.qrc" << "images.expected";

    const QString sizesPath = m_dataPath + QLatin1String("/sizes");
    QTest::newRow("size-0") << sizesPath << "size-0.qrc" << "size-0.expected";
    QTest::newRow("size-1") << sizesPath << "size-1.qrc" << "size-1.expected";
    QTest::newRow("size-2-0-35-1") << sizesPath << "size-2-0-35-1.qrc" << "size-2-0-35-1.expected";
}

static QStringList readLinesFromFile(const QString &fileName,
                                     Qt::SplitBehavior splitBehavior)
{
    QFile file(fileName);

    bool ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!ok) {
        QWARN(qPrintable(QString::fromLatin1("Could not open testdata file %1: %2")
                         .arg(fileName, file.errorString())));
    }

    return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), splitBehavior);
}

void tst_rcc::rcc()
{
    QFETCH(QString, directory);
    QFETCH(QString, qrcfile);
    QFETCH(QString, expected);

    // If the file expectedoutput.txt exists, compare the
    // console output with the content of that file

    // Launch; force no compression, otherwise the output would be different
    // depending on the compression algorithm we're using
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(m_rcc, { "-no-compress", qrcfile });
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit,
             msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0,
             msgProcessFailed(process).constData());

    const QChar cr = QLatin1Char('\r');
    const QString err = QString::fromLocal8Bit(process.readAllStandardError()).remove(cr);
    const QString out = QString::fromLatin1(process.readAllStandardOutput()).remove(cr);

    if (!err.isEmpty()) {
        qDebug() << "UNEXPECTED STDERR CONTENTS: " << err;
        QFAIL("UNEXPECTED STDERR CONTENTS");
    }

    const QChar nl = QLatin1Char('\n');
    const QStringList actualLines = out.split(nl);

    const QStringList expectedLines =
        readLinesFromFile(directory + QLatin1Char('/') + expected, Qt::KeepEmptyParts);
    QVERIFY(!expectedLines.isEmpty());

    const QString diff = doCompare(actualLines, expectedLines, directory);
    if (diff.size())
        QFAIL(qPrintable(diff));
}

static QStringMap readExpectedFiles(const QString &fileName)
{
    QStringMap expectedFiles;

    QStringList lines = readLinesFromFile(fileName, Qt::SkipEmptyParts);
    foreach (const QString &line, lines) {
        QString resourceFileName = line.section(QLatin1Char(' '), 0, 0, QString::SectionSkipEmpty);
        QString actualFileName = line.section(QLatin1Char(' '), 1, 1, QString::SectionSkipEmpty);
        expectedFiles[resourceFileName] = actualFileName;
    }

    return expectedFiles;
}

/*
    The following test looks for all *.qrc files under data/binary/. For each
    .qrc file found, these files are processed (assuming the file found is
    called "base.qrc"):

    - base.qrc : processed by rcc; creates base.rcc
    - base.locale : (optional) list of locales to test, one per line
    - base.expected : list of pairs (file path in resource, path to real file),
        one per line; the pair separated by a whitespace; the paths to real files
        relative to data/binary/ (for testing the C locale)
    - base.localeName.expected : for each localeName in the base.locale file,
        as the above .expected file
*/

void tst_rcc::binary_data()
{
    QTest::addColumn<QString>("resourceFile");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QString>("baseDirectory");
    QTest::addColumn<QStringMap>("expectedFiles");

    QString dataPath = m_dataPath + QLatin1String("/binary/");

    QDirIterator iter(dataPath, QStringList() << QLatin1String("*.qrc"));
    while (iter.hasNext())
    {
        iter.next();
        QFileInfo qrcFileInfo = iter.fileInfo();
        QString absoluteBaseName = QFileInfo(qrcFileInfo.absolutePath(), qrcFileInfo.baseName()).absoluteFilePath();
        QString rccFileName = absoluteBaseName + QLatin1String(".rcc");

        // same as above: force no compression
        QProcess rccProcess;
        rccProcess.setWorkingDirectory(dataPath);
        rccProcess.start(m_rcc, { "-binary", "-no-compress", "-o", rccFileName, qrcFileInfo.absoluteFilePath() });
        QVERIFY2(rccProcess.waitForStarted(), msgProcessStartFailed(rccProcess).constData());
        if (!rccProcess.waitForFinished()) {
            rccProcess.kill();
            QFAIL(msgProcessTimeout(rccProcess).constData());
        }
        QVERIFY2(rccProcess.exitStatus() == QProcess::NormalExit,
                 msgProcessCrashed(rccProcess).constData());
        QVERIFY2(rccProcess.exitCode() == 0,
                 msgProcessFailed(rccProcess).constData());

        QByteArray output = rccProcess.readAllStandardOutput();
        if (!output.isEmpty())
            qWarning("rcc stdout: %s", output.constData());

        output = rccProcess.readAllStandardError();
        if (!output.isEmpty())
            qWarning("rcc stderr: %s", output.constData());

        QString localeFileName = absoluteBaseName + QLatin1String(".locale");
        QFile localeFile(localeFileName);
        if (localeFile.exists()) {
            QStringList locales = readLinesFromFile(localeFileName, Qt::SkipEmptyParts);
            foreach (const QString &locale, locales) {
                QString expectedFileName = QString::fromLatin1("%1.%2.%3").arg(absoluteBaseName, locale, QLatin1String("expected"));
                QStringMap expectedFiles = readExpectedFiles(expectedFileName);
                QTest::newRow(qPrintable(qrcFileInfo.baseName() + QLatin1Char('_') + locale)) << rccFileName
                                                                                              << QLocale(locale)
                                                                                              << dataPath
                                                                                              << expectedFiles;
            }
        }

        // always test for the C locale as well
        QString expectedFileName = absoluteBaseName + QLatin1String(".expected");
        QStringMap expectedFiles = readExpectedFiles(expectedFileName);
        QTest::newRow(qPrintable(qrcFileInfo.baseName() + QLatin1String("_C"))) << rccFileName
                                                                                << QLocale::c()
                                                                                << dataPath
                                                                                << expectedFiles;
    }
}

void tst_rcc::binary()
{
    QFETCH(QString, baseDirectory);
    QFETCH(QString, resourceFile);
    QFETCH(QLocale, locale);
    QFETCH(QStringMap, expectedFiles);

    const QString rootPrefix = QLatin1String("/test_root/");
    const QString resourceRootPrefix = QLatin1Char(':') + rootPrefix;

    QLocale oldDefaultLocale;
    QLocale::setDefault(locale);
    QVERIFY(QFile::exists(resourceFile));
    QVERIFY(QResource::registerResource(resourceFile, rootPrefix));

    { // need to destroy the iterators on the resource, in order to be able to unregister it

    // read all the files inside the resources
    QDirIterator iter(resourceRootPrefix, QDir::Files, QDirIterator::Subdirectories);
    QList<QString> filesFound;
    while (iter.hasNext())
        filesFound << iter.next();

    // add the test root prefix to the expected file names
    QList<QString> expectedFileNames = expectedFiles.keys();
    for (QList<QString>::iterator i = expectedFileNames.begin(); i < expectedFileNames.end(); ++i) {
        // poor man's canonicalPath, which doesn't work with resources
        if ((*i).startsWith(QLatin1Char('/')))
            (*i).remove(0, 1);
        *i = resourceRootPrefix + *i;
    }

    // check that we have all (and only) the expected files
    std::sort(filesFound.begin(), filesFound.end());
    std::sort(expectedFileNames.begin(), expectedFileNames.end());
    QCOMPARE(filesFound, expectedFileNames);

    // now actually check the file contents
    QDir directory(baseDirectory);
    for (QStringMap::const_iterator i = expectedFiles.constBegin(); i != expectedFiles.constEnd(); ++i) {
        QString resourceFileName = i.key();
        QString actualFileName = i.value();

        QFile resourceFile(resourceRootPrefix + resourceFileName);
        QVERIFY(resourceFile.open(QIODevice::ReadOnly));
        QByteArray resourceData = resourceFile.readAll();
        resourceFile.close();

        QFile actualFile(QFileInfo(directory, actualFileName).absoluteFilePath());
        QVERIFY(actualFile.open(QIODevice::ReadOnly));
        QByteArray actualData = actualFile.readAll();
        actualFile.close();
        QCOMPARE(resourceData, actualData);
    }

    }

    QVERIFY(QResource::unregisterResource(resourceFile, rootPrefix));
    QLocale::setDefault(oldDefaultLocale);
}

void tst_rcc::readback_data()
{
    QTest::addColumn<QString>("resourceName");
    QTest::addColumn<QString>("fileSystemName");

    QTest::newRow("data-0")   << ":data/data-0.txt"            << "sizes/data/data-0.txt";
    QTest::newRow("data-1")   << ":data/data-1.txt"            << "sizes/data/data-1.txt";
    QTest::newRow("data-2")   << ":data/data-2.txt"            << "sizes/data/data-2.txt";
    QTest::newRow("data-35")  << ":data/data-35.txt"           << "sizes/data/data-35.txt";
    QTest::newRow("circle")   << ":images/circle.png"          << "images/images/circle.png";
    QTest::newRow("square")   << ":images/square.png"          << "images/images/square.png";
    QTest::newRow("triangle") << ":images/subdir/triangle.png"
                                  << "images/images/subdir/triangle.png";
}

void tst_rcc::readback()
{
    QFETCH(QString, resourceName);
    QFETCH(QString, fileSystemName);

    QFile resourceFile(resourceName);
    QVERIFY(resourceFile.open(QIODevice::ReadOnly));
    QByteArray resourceData = resourceFile.readAll();
    resourceFile.close();

    QFile fileSystemFile(m_dataPath + QLatin1Char('/') + fileSystemName);
    QVERIFY(fileSystemFile.open(QIODevice::ReadOnly));
    QByteArray fileSystemData = fileSystemFile.readAll();
    fileSystemFile.close();

    QCOMPARE(resourceData, fileSystemData);
}

void tst_rcc::depFileGeneration_data()
{
    QTest::addColumn<QString>("qrcfile");
    QTest::addColumn<QString>("depfile");
    QTest::addColumn<QString>("expected");

    QTest::newRow("simple") << "simple.qrc" << "simple.d" << "simple.d.expected";
    QTest::newRow("specialchar") << "specialchar.qrc" << "specialchar.d" << "specialchar.d.expected";
}

void tst_rcc::depFileGeneration()
{
    QFETCH(QString, qrcfile);
    QFETCH(QString, depfile);
    QFETCH(QString, expected);
    const QString directory = m_dataPath + QLatin1String("/depfile");

    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(m_rcc, { "-d", depfile, "-o", qrcfile + ".cpp", qrcfile });
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit,
             msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0,
             msgProcessFailed(process).constData());

    QFile depFileOutput(directory + QLatin1String("/") + depfile);
    QVERIFY(depFileOutput.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray depFileData = depFileOutput.readAll();
    depFileOutput.close();

    QFile depFileExpected(directory + QLatin1String("/") + expected);
    QVERIFY(depFileExpected.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray expectedData = depFileExpected.readAll();
    depFileExpected.close();

    QCOMPARE(depFileData, expectedData);
}

void tst_rcc::python()
{
    const QString path = m_dataPath + QLatin1String("/sizes");
    const QString testFileRoot = path + QLatin1String("/size-2-0-35-1");
    const QString qrcFile = testFileRoot + QLatin1String(".qrc");
    const QString expectedFile = testFileRoot + QLatin1String("_python.expected");
    const QString actualFile = testFileRoot + QLatin1String(".rcc");

    QProcess process;
    process.setWorkingDirectory(path);
    process.start(m_rcc, { "-g", "python", "-o", actualFile, qrcFile});
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit,
             msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0,
             msgProcessFailed(process).constData());

    const auto actualLines = readLinesFromFile(actualFile, Qt::KeepEmptyParts);
    QVERIFY(!actualLines.isEmpty());
    const auto expectedLines = readLinesFromFile(expectedFile, Qt::KeepEmptyParts);
    QVERIFY(!expectedLines.isEmpty());
    const QString diff = doCompare(actualLines, expectedLines, path);
    if (!diff.isEmpty())
        QFAIL(qPrintable(diff));
}

void tst_rcc::cleanupTestCase()
{
    QDir dataDir(m_dataPath + QLatin1String("/binary"));
    QFileInfoList entries = dataDir.entryInfoList(QStringList() << QLatin1String("*.rcc"));
    QDir dataDepDir(m_dataPath + QLatin1String("/depfile"));
    entries += dataDepDir.entryInfoList({QLatin1String("*.d"), QLatin1String("*.qrc.cpp")});
    foreach (const QFileInfo &entry, entries)
        QFile::remove(entry.absoluteFilePath());
}

QTEST_MAIN(tst_rcc)

#include "tst_rcc.moc"
