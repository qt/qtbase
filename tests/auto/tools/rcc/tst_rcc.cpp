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

class tst_rcc : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void rcc_data();
    void rcc();
    void binary_data();
    void binary();

    void cleanupTestCase();

private:
    QString m_rcc;
};

void tst_rcc::initTestCase()
{
    // rcc uses a QHash to store files in the resource system.
    // we must force a certain hash order when testing or tst_rcc will fail, see QTBUG-25078
    QVERIFY(qputenv("QT_RCC_TEST", "1"));
    m_rcc = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/rcc");
}

QString findExpectedFile(const QString &base)
{
    QString expectedrccfile = base;

    // Must be updated with each minor release.
    if (QFileInfo(expectedrccfile + QLatin1String(".450")).exists())
        expectedrccfile += QLatin1String(".450");

    return expectedrccfile;
}

static QString doCompare(const QStringList &actual, const QStringList &expected)
{
    if (actual.size() != expected.size()) {
        return QString("Length count different: actual: %1, expected: %2")
            .arg(actual.size()).arg(expected.size());
    }

    QByteArray ba;
    for (int i = 0, n = expected.size(); i != n; ++i) {
        QString expectedLine = expected.at(i);
        if (expectedLine.startsWith("IGNORE:"))
            continue;
        if (expectedLine.startsWith("TIMESTAMP:")) {
            const QString relativePath = expectedLine.mid(strlen("TIMESTAMP:"));
            const quint64 timeStamp = QFileInfo(relativePath).lastModified().toMSecsSinceEpoch();
            expectedLine.clear();
            for (int shift = 56; shift >= 0; shift -= 8) {
                expectedLine.append(QLatin1String("0x"));
                expectedLine.append(QString::number(quint8(timeStamp >> shift), 16));
                expectedLine.append(QLatin1Char(','));
            }
        }
        if (expectedLine != actual.at(i)) {
            qDebug() << "LINES" << i << "DIFFER";
            ba.append(
             "\n<<<<<< actual\n" + actual.at(i) + "\n======\n" + expectedLine
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

    QString dataPath = QFINDTESTDATA("data/images/");
    if (dataPath.isEmpty())
        QFAIL("data path not found");
    QTest::newRow("images") << dataPath << "images.qrc" << "images.expected";
}

void tst_rcc::rcc()
{
    QFETCH(QString, directory);
    QFETCH(QString, qrcfile);
    QFETCH(QString, expected);

    if (!QDir::setCurrent(directory)) {
        QString message = QString::fromLatin1("Unable to cd from '%1' to '%2'").arg(QDir::currentPath(), directory);
        QFAIL(qPrintable(message));
    }

    // If the file expectedoutput.txt exists, compare the
    // console output with the content of that file
    const QString expected2 = findExpectedFile(expected);
    QFile expectedFile(expected2);
    if (!expectedFile.exists()) {
        qDebug() << "NO EXPECTATIONS? " << expected2;
        return;
    }

    // Launch
    QProcess process;
    process.start(m_rcc, QStringList(qrcfile));
    if (!process.waitForFinished()) {
        const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
        QString message = QString::fromLatin1("'%1' could not be found when run from '%2'. Path: '%3' ").
                          arg(m_rcc, QDir::currentPath(), path);
        QFAIL(qPrintable(message));
    }
    const QChar cr = QLatin1Char('\r');
    const QString err = QString::fromLocal8Bit(process.readAllStandardError()).remove(cr);
    const QString out = QString::fromLatin1(process.readAllStandardOutput()).remove(cr);

    if (!err.isEmpty()) {
        qDebug() << "UNEXPECTED STDERR CONTENTS: " << err;
        QFAIL("UNEXPECTED STDERR CONTENTS");
    }

    const QChar nl = QLatin1Char('\n');
    const QStringList actualLines = out.split(nl);

    QVERIFY(expectedFile.open(QIODevice::ReadOnly|QIODevice::Text));
    const QStringList expectedLines = QString::fromLatin1(expectedFile.readAll()).split(nl);

    const QString diff = doCompare(actualLines, expectedLines);
    if (diff.size())
        QFAIL(qPrintable(diff));
}



static void createRccBinaryData(const QString &rcc, const QString &baseDir,
    const QString &qrcFileName, const QString &rccFileName)
{
    QString currentDir = QDir::currentPath();
    QDir::setCurrent(baseDir);

    QProcess rccProcess;
    rccProcess.start(rcc, QStringList() << "-binary" << "-o" << rccFileName << qrcFileName);
    bool ok = rccProcess.waitForFinished();
    if (!ok) {
        QString errorString = QString::fromLatin1("Could not start rcc (is it in PATH?): %1").arg(rccProcess.errorString());
        QFAIL(qPrintable(errorString));
    }

    QByteArray output = rccProcess.readAllStandardOutput();
    if (!output.isEmpty()) {
        QString errorMessage = QString::fromLatin1("rcc stdout: %1").arg(QString::fromLocal8Bit(output));
        QWARN(qPrintable(errorMessage));
    }

    output = rccProcess.readAllStandardError();
    if (!output.isEmpty()) {
        QString errorMessage = QString::fromLatin1("rcc stderr: %1").arg(QString::fromLocal8Bit(output));
        QWARN(qPrintable(errorMessage));
    }

    QDir::setCurrent(currentDir);
}

static QStringList readLinesFromFile(const QString &fileName)
{
    QFile file(fileName);

    bool ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!ok)
        QWARN(qPrintable(QString::fromLatin1("Could not open testdata file %1: %2").arg(fileName, file.errorString())));

    QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), QString::SkipEmptyParts);
    return lines;
}

static QStringMap readExpectedFiles(const QString &fileName)
{
    QStringMap expectedFiles;

    QStringList lines = readLinesFromFile(fileName);
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

    QString dataPath = QFINDTESTDATA("data/binary/");
    if (dataPath.isEmpty())
        QFAIL("data path not found");

    QDirIterator iter(dataPath, QStringList() << QLatin1String("*.qrc"));
    while (iter.hasNext())
    {
        iter.next();
        QFileInfo qrcFileInfo = iter.fileInfo();
        QString absoluteBaseName = QFileInfo(qrcFileInfo.absolutePath(), qrcFileInfo.baseName()).absoluteFilePath();
        QString rccFileName = absoluteBaseName + QLatin1String(".rcc");
        createRccBinaryData(m_rcc, dataPath, qrcFileInfo.absoluteFilePath(), rccFileName);

        QString localeFileName = absoluteBaseName + QLatin1String(".locale");
        QFile localeFile(localeFileName);
        if (localeFile.exists()) {
            QStringList locales = readLinesFromFile(localeFileName);
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


void tst_rcc::cleanupTestCase()
{
    QString dataPath = QFINDTESTDATA("data/binary/");
    if (dataPath.isEmpty())
        return;
    QDir dataDir(dataPath);
    QFileInfoList entries = dataDir.entryInfoList(QStringList() << QLatin1String("*.rcc"));
    foreach (const QFileInfo &entry, entries)
        QFile::remove(entry.absoluteFilePath());
}

QTEST_MAIN(tst_rcc)

#include "tst_rcc.moc"
