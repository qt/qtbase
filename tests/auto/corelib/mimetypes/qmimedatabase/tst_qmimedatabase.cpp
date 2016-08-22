/****************************************************************************
**
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

#include <qmimedatabase.h>

#include "qstandardpaths.h"

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtConcurrent/QtConcurrentRun>

#include <QtTest/QtTest>

static const char *const additionalMimeFiles[] = {
    "yast2-metapackage-handler-mimetypes.xml",
    "qml-again.xml",
    "text-x-objcsrc.xml",
    "invalid-magic1.xml",
    "invalid-magic2.xml",
    "invalid-magic3.xml",
    0
};

#define RESOURCE_PREFIX ":/qt-project.org/qmime/"

void initializeLang()
{
    qputenv("LC_ALL", "");
    qputenv("LANG", "C");
    QCoreApplication::setApplicationName("tst_qmimedatabase"); // temporary directory pattern
}

static inline QString testSuiteWarning()
{

    QString result;
    QTextStream str(&result);
    str << "\nCannot find the shared-mime-info test suite\nstarting from: "
        << QDir::toNativeSeparators(QDir::currentPath()) << "\n"
           "cd " << QDir::toNativeSeparators(QStringLiteral("tests/auto/corelib/mimetypes/qmimedatabase")) << "\n"
           "wget http://cgit.freedesktop.org/xdg/shared-mime-info/snapshot/Release-1-0.zip\n"
           "unzip Release-1-0.zip\n";
#ifdef Q_OS_WIN
    str << "mkdir testfiles\nxcopy /s Release-1-0\\tests testfiles\n";
#else
    str << "ln -s Release-1-0/tests testfiles\n";
#endif
    return result;
}

static bool copyResourceFile(const QString &sourceFileName, const QString &targetFileName,
                             QString *errorMessage)
{

    QFile sourceFile(sourceFileName);
    if (!sourceFile.exists()) {
        *errorMessage = QDir::toNativeSeparators(sourceFileName) + QLatin1String(" does not exist.");
        return false;
    }
    if (!sourceFile.copy(targetFileName)) {
        *errorMessage = QLatin1String("Cannot copy ")
            + QDir::toNativeSeparators(sourceFileName) + QLatin1String(" to ")
            + QDir::toNativeSeparators(targetFileName) + QLatin1String(": ")
            + sourceFile.errorString();
        return false;
    }
    // QFile::copy() sets the permissions of the source file which are read-only for
    // resource files. Set write permission to enable deletion of the temporary directory.
    QFile targetFile(targetFileName);
    if (!targetFile.setPermissions(targetFile.permissions() | QFileDevice::WriteUser)) {
        *errorMessage = QLatin1String("Cannot set write permission on ")
            + QDir::toNativeSeparators(targetFileName) + QLatin1String(": ")
            + targetFile.errorString();
        return false;
    }
    return true;
}

// Set LANG before QCoreApplication is created
Q_CONSTRUCTOR_FUNCTION(initializeLang)

static QString seedAndTemplate()
{
    qsrand(QDateTime::currentSecsSinceEpoch());
    return QDir::tempPath() + "/tst_qmimedatabase-XXXXXX";
}

tst_QMimeDatabase::tst_QMimeDatabase()
    : m_temporaryDir(seedAndTemplate())
{
}

void tst_QMimeDatabase::initTestCase()
{
    QLocale::setDefault(QLocale::c());
    QVERIFY2(m_temporaryDir.isValid(), qPrintable(m_temporaryDir.errorString()));
    QStandardPaths::setTestModeEnabled(true);
    m_localMimeDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/mime";
    if (QDir(m_localMimeDir).exists()) {
        QVERIFY2(QDir(m_localMimeDir).removeRecursively(), qPrintable(m_localMimeDir + ": " + qt_error_string()));
    }
    QString errorMessage;

#ifdef USE_XDG_DATA_DIRS
    // Create a temporary "global" XDG data dir for later use
    // It will initially contain a copy of freedesktop.org.xml
    QVERIFY2(m_temporaryDir.isValid(),
             ("Could not create temporary subdir: " + m_temporaryDir.errorString()).toUtf8());
    const QDir here = QDir(m_temporaryDir.path());
    m_globalXdgDir = m_temporaryDir.path() + QStringLiteral("/global");
    const QString globalPackageDir = m_globalXdgDir + QStringLiteral("/mime/packages");
    QVERIFY(here.mkpath(globalPackageDir));

    qputenv("XDG_DATA_DIRS", QFile::encodeName(m_globalXdgDir));
    qDebug() << "\nGlobal XDG_DATA_DIRS: " << m_globalXdgDir;

    const QString freeDesktopXml = QStringLiteral("freedesktop.org.xml");
    const QString xmlFileName = QLatin1String(RESOURCE_PREFIX) + freeDesktopXml;
    const QString xmlTargetFileName = globalPackageDir + QLatin1Char('/') + freeDesktopXml;
    QVERIFY2(copyResourceFile(xmlFileName, xmlTargetFileName, &errorMessage), qPrintable(errorMessage));
#endif

    m_testSuite = QFINDTESTDATA("testfiles");
    if (m_testSuite.isEmpty())
        qWarning("%s", qPrintable(testSuiteWarning()));

    errorMessage = QString::fromLatin1("Cannot find '%1'");
    for (uint i = 0; i < sizeof additionalMimeFiles / sizeof additionalMimeFiles[0] - 1; i++) {
        const QString resourceFilePath = QString::fromLatin1(RESOURCE_PREFIX) + QLatin1String(additionalMimeFiles[i]);
        QVERIFY2(QFile::exists(resourceFilePath), qPrintable(errorMessage.arg(resourceFilePath)));
        m_additionalMimeFileNames.append(QLatin1String(additionalMimeFiles[i]));
        m_additionalMimeFilePaths.append(resourceFilePath);
    }

    initTestCaseInternal();
    m_isUsingCacheProvider = !qEnvironmentVariableIsSet("QT_NO_MIME_CACHE");
}

void tst_QMimeDatabase::init()
{
    // clean up local data from previous runs
    QDir(m_localMimeDir).removeRecursively();
}

void tst_QMimeDatabase::cleanupTestCase()
{
    QDir(m_localMimeDir).removeRecursively();
}

void tst_QMimeDatabase::mimeTypeForName()
{
    QMimeDatabase db;
    QMimeType s0 = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QVERIFY(s0.isValid());
    QCOMPARE(s0.name(), QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0.comment(), QString::fromLatin1("empty document"));

    QMimeType s0Again = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0Again.name(), s0.name());

    QMimeType s1 = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    QVERIFY(s1.isValid());
    QCOMPARE(s1.name(), QString::fromLatin1("text/plain"));
    //qDebug("Comment is %s", qPrintable(s1.comment()));

    QMimeType krita = db.mimeTypeForName(QString::fromLatin1("application/x-krita"));
    QVERIFY(krita.isValid());

    // Test <comment> parsing with application/rdf+xml which has the english comment after the other ones
    QMimeType rdf = db.mimeTypeForName(QString::fromLatin1("application/rdf+xml"));
    QVERIFY(rdf.isValid());
    QCOMPARE(rdf.comment(), QString::fromLatin1("RDF file"));

    QMimeType bzip2 = db.mimeTypeForName(QString::fromLatin1("application/x-bzip2"));
    QVERIFY(bzip2.isValid());
    QCOMPARE(bzip2.comment(), QString::fromLatin1("Bzip archive"));

    QMimeType defaultMime = db.mimeTypeForName(QString::fromLatin1("application/octet-stream"));
    QVERIFY(defaultMime.isValid());
    QVERIFY(defaultMime.isDefault());

    QMimeType doesNotExist = db.mimeTypeForName(QString::fromLatin1("foobar/x-doesnot-exist"));
    QVERIFY(!doesNotExist.isValid());

    // TODO move to findByFile
#ifdef Q_OS_LINUX
    QString exePath = QStandardPaths::findExecutable(QLatin1String("ls"));
    if (exePath.isEmpty())
        qWarning() << "ls not found";
    else {
        const QString executableType = QString::fromLatin1("application/x-executable");
        //QTest::newRow("executable") << exePath << executableType;
        QCOMPARE(db.mimeTypeForFile(exePath).name(), executableType);
    }
#endif

}

void tst_QMimeDatabase::mimeTypeForFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedMimeType");

    QTest::newRow("text") << "textfile.txt" << "text/plain";
    QTest::newRow("case-insensitive search") << "textfile.TxT" << "text/plain";

    // Needs shared-mime-info > 0.91. Earlier versions wrote .Z to the mime.cache file...
    //QTest::newRow("case-insensitive match on a non-lowercase glob") << "foo.z" << "application/x-compress";

    QTest::newRow("case-sensitive uppercase match") << "textfile.C" << "text/x-c++src";
    QTest::newRow("case-sensitive lowercase match") << "textfile.c" << "text/x-csrc";
    QTest::newRow("case-sensitive long-extension match") << "foo.PS.gz" << "application/x-gzpostscript";
    QTest::newRow("case-sensitive-only match") << "core" << "application/x-core";
    QTest::newRow("case-sensitive-only match") << "Core" << "application/octet-stream"; // #198477

    QTest::newRow("desktop file") << "foo.desktop" << "application/x-desktop";
    QTest::newRow("old kdelnk file is x-desktop too") << "foo.kdelnk" << "application/x-desktop";
    QTest::newRow("double-extension file") << "foo.tar.bz2" << "application/x-bzip-compressed-tar";
    QTest::newRow("single-extension file") << "foo.bz2" << "application/x-bzip";
    QTest::newRow(".doc should assume msword") << "somefile.doc" << "application/msword"; // #204139
    QTest::newRow("glob that uses [] syntax, 1") << "Makefile" << "text/x-makefile";
    QTest::newRow("glob that uses [] syntax, 2") << "makefile" << "text/x-makefile";
    QTest::newRow("glob that ends with *, no extension") << "README" << "text/x-readme";
    QTest::newRow("glob that ends with *, extension") << "README.foo" << "text/x-readme";
    QTest::newRow("glob that ends with *, also matches *.txt. Higher weight wins.") << "README.txt" << "text/plain";
    QTest::newRow("glob that ends with *, also matches *.nfo. Higher weight wins.") << "README.nfo" << "text/x-nfo";
    // fdo bug 15436, needs shared-mime-info >= 0.40 (and this tests the globs2-parsing code).
    QTest::newRow("glob that ends with *, also matches *.pdf. *.pdf has higher weight") << "README.pdf" << "application/pdf";
    QTest::newRow("directory") << "/" << "inode/directory";
    QTest::newRow("doesn't exist, no extension") << "IDontExist" << "application/octet-stream";
    QTest::newRow("doesn't exist but has known extension") << "IDontExist.txt" << "text/plain";
    QTest::newRow("empty") << "" << "application/octet-stream";
}

static inline QByteArray msgMimeTypeForFileNameFailed(const QList<QMimeType> &actual,
                                                      const QString &expected)
{
    QByteArray result = "Actual (";
    foreach (const QMimeType &m, actual) {
        result += m.name().toLocal8Bit();
        result +=  ' ';
    }
    result +=  ") , expected: ";
    result +=  expected.toLocal8Bit();
    return result;
}

void tst_QMimeDatabase::mimeTypeForFileName()
{
    QFETCH(QString, fileName);
    QFETCH(QString, expectedMimeType);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QCOMPARE(mime.name(), expectedMimeType);

    QList<QMimeType> mimes = db.mimeTypesForFileName(fileName);
    if (expectedMimeType == "application/octet-stream") {
        QVERIFY(mimes.isEmpty());
    } else {
        QVERIFY2(!mimes.isEmpty(), msgMimeTypeForFileNameFailed(mimes, expectedMimeType).constData());
        QVERIFY2(mimes.count() == 1, msgMimeTypeForFileNameFailed(mimes, expectedMimeType).constData());
        QCOMPARE(mimes.first().name(), expectedMimeType);
    }
}

void tst_QMimeDatabase::mimeTypesForFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QStringList>("expectedMimeTypes");

    QTest::newRow("txt, 1 hit") << "foo.txt" << (QStringList() << "text/plain");
    QTest::newRow("txtfoobar, 0 hit") << "foo.foobar" << QStringList();
    QTest::newRow("m, 2 hits") << "foo.m" << (QStringList() << "text/x-matlab" << "text/x-objcsrc");
    QTest::newRow("sub, 3 hits") << "foo.sub" << (QStringList() << "text/x-microdvd" << "text/x-mpsub" << "text/x-subviewer");
}

void tst_QMimeDatabase::mimeTypesForFileName()
{
    QFETCH(QString, fileName);
    QFETCH(QStringList, expectedMimeTypes);
    QMimeDatabase db;
    QList<QMimeType> mimes = db.mimeTypesForFileName(fileName);
    QStringList mimeNames;
    foreach (const QMimeType &mime, mimes)
        mimeNames.append(mime.name());
    QCOMPARE(mimeNames, expectedMimeTypes);
}

void tst_QMimeDatabase::inheritance()
{
    QMimeDatabase db;

    // All file-like mimetypes inherit from octet-stream
    const QMimeType wordperfect = db.mimeTypeForName(QString::fromLatin1("application/vnd.wordperfect"));
    QVERIFY(wordperfect.isValid());
    QCOMPARE(wordperfect.parentMimeTypes().join(QString::fromLatin1(",")), QString::fromLatin1("application/octet-stream"));
    QVERIFY(wordperfect.inherits(QLatin1String("application/octet-stream")));

    QVERIFY(db.mimeTypeForName(QString::fromLatin1("image/svg+xml-compressed")).inherits(QLatin1String("application/x-gzip")));

    // Check that msword derives from ole-storage
    const QMimeType msword = db.mimeTypeForName(QString::fromLatin1("application/msword"));
    QVERIFY(msword.isValid());
    const QMimeType olestorage = db.mimeTypeForName(QString::fromLatin1("application/x-ole-storage"));
    QVERIFY(olestorage.isValid());
    QVERIFY(msword.inherits(olestorage.name()));
    QVERIFY(msword.inherits(QLatin1String("application/octet-stream")));

    const QMimeType directory = db.mimeTypeForName(QString::fromLatin1("inode/directory"));
    QVERIFY(directory.isValid());
    QCOMPARE(directory.parentMimeTypes().count(), 0);
    QVERIFY(!directory.inherits(QLatin1String("application/octet-stream")));

    // Check that text/x-patch knows that it inherits from text/plain (it says so explicitly)
    const QMimeType plain = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    const QMimeType derived = db.mimeTypeForName(QString::fromLatin1("text/x-patch"));
    QVERIFY(derived.isValid());
    QCOMPARE(derived.parentMimeTypes().join(QString::fromLatin1(",")), plain.name());
    QVERIFY(derived.inherits(QLatin1String("text/plain")));
    QVERIFY(derived.inherits(QLatin1String("application/octet-stream")));

    // Check that application/x-shellscript inherits from application/x-executable
    // (Otherwise KRun cannot start shellscripts...)
    // This is a test for multiple inheritance...
    const QMimeType shellscript = db.mimeTypeForName(QString::fromLatin1("application/x-shellscript"));
    QVERIFY(shellscript.isValid());
    QVERIFY(shellscript.inherits(QLatin1String("text/plain")));
    QVERIFY(shellscript.inherits(QLatin1String("application/x-executable")));
    const QStringList shellParents = shellscript.parentMimeTypes();
    QVERIFY(shellParents.contains(QLatin1String("text/plain")));
    QVERIFY(shellParents.contains(QLatin1String("application/x-executable")));
    QCOMPARE(shellParents.count(), 2); // only the above two
    const QStringList allShellAncestors = shellscript.allAncestors();
    QVERIFY(allShellAncestors.contains(QLatin1String("text/plain")));
    QVERIFY(allShellAncestors.contains(QLatin1String("application/x-executable")));
    QVERIFY(allShellAncestors.contains(QLatin1String("application/octet-stream")));
    // Must be least-specific last, i.e. breadth first.
    QCOMPARE(allShellAncestors.last(), QString::fromLatin1("application/octet-stream"));

    const QStringList allSvgAncestors = db.mimeTypeForName(QString::fromLatin1("image/svg+xml")).allAncestors();
    QCOMPARE(allSvgAncestors, QStringList() << QLatin1String("application/xml") << QLatin1String("text/plain") << QLatin1String("application/octet-stream"));

    // Check that text/x-mrml knows that it inherits from text/plain (implicitly)
    const QMimeType mrml = db.mimeTypeForName(QString::fromLatin1("text/x-mrml"));
    QVERIFY(mrml.isValid());
    QVERIFY(mrml.inherits(QLatin1String("text/plain")));
    QVERIFY(mrml.inherits(QLatin1String("application/octet-stream")));

    // Check that msword-template inherits msword
    const QMimeType mswordTemplate = db.mimeTypeForName(QString::fromLatin1("application/msword-template"));
    QVERIFY(mswordTemplate.isValid());
    QVERIFY(mswordTemplate.inherits(QLatin1String("application/msword")));
}

void tst_QMimeDatabase::aliases()
{
    QMimeDatabase db;

    const QMimeType canonical = db.mimeTypeForName(QString::fromLatin1("application/xml"));
    QVERIFY(canonical.isValid());

    QMimeType resolvedAlias = db.mimeTypeForName(QString::fromLatin1("text/xml"));
    QVERIFY(resolvedAlias.isValid());
    QCOMPARE(resolvedAlias.name(), QString::fromLatin1("application/xml"));

    QVERIFY(resolvedAlias.inherits(QLatin1String("application/xml")));
    QVERIFY(canonical.inherits(QLatin1String("text/xml")));

    // Test for kde bug 197346: does nspluginscan see that audio/mp3 already exists?
    bool mustWriteMimeType = !db.mimeTypeForName(QString::fromLatin1("audio/mp3")).isValid();
    QVERIFY(!mustWriteMimeType);
}

void tst_QMimeDatabase::listAliases_data()
{
    QTest::addColumn<QString>("inputMime");
    QTest::addColumn<QString>("expectedAliases");

    QTest::newRow("csv") << "text/csv" << "text/x-csv,text/x-comma-separated-values";
    QTest::newRow("xml") << "application/xml" << "text/xml";
    QTest::newRow("xml2") << "text/xml" /* gets resolved to application/xml */ << "text/xml";
    QTest::newRow("no_mime") << "message/news" << "";
}

void tst_QMimeDatabase::listAliases()
{
    QFETCH(QString, inputMime);
    QFETCH(QString, expectedAliases);
    QMimeDatabase db;
    QStringList expectedAliasesList = expectedAliases.split(',', QString::SkipEmptyParts);
    expectedAliasesList.sort();
    QMimeType mime = db.mimeTypeForName(inputMime);
    QVERIFY(mime.isValid());
    QStringList aliasList = mime.aliases();
    aliasList.sort();
    QCOMPARE(aliasList, expectedAliasesList);
}

void tst_QMimeDatabase::icons()
{
    QMimeDatabase db;
    QMimeType directory = db.mimeTypeForFile(QString::fromLatin1("/"));
    QCOMPARE(directory.name(), QString::fromLatin1("inode/directory"));
    QCOMPARE(directory.iconName(), QString::fromLatin1("inode-directory"));
    QCOMPARE(directory.genericIconName(), QString::fromLatin1("inode-x-generic"));

    QMimeType pub = db.mimeTypeForFile(QString::fromLatin1("foo.epub"), QMimeDatabase::MatchExtension);
    QCOMPARE(pub.name(), QString::fromLatin1("application/epub+zip"));
    QCOMPARE(pub.iconName(), QString::fromLatin1("application-epub+zip"));
    QCOMPARE(pub.genericIconName(), QString::fromLatin1("x-office-document"));
}

void tst_QMimeDatabase::comment()
{
    struct RestoreLocale
    {
        ~RestoreLocale() { QLocale::setDefault(QLocale::c()); }
    } restoreLocale;

    QLocale::setDefault(QLocale("de"));
    QMimeDatabase db;
    QMimeType directory = db.mimeTypeForName(QStringLiteral("inode/directory"));
    QCOMPARE(directory.comment(), QStringLiteral("Ordner"));
    QLocale::setDefault(QLocale("fr"));
    QCOMPARE(directory.comment(), QStringLiteral("dossier"));
}

// In here we do the tests that need some content in a temporary file.
// This could also be added to shared-mime-info's testsuite...
void tst_QMimeDatabase::mimeTypeForFileWithContent()
{
    QMimeDatabase db;
    QMimeType mime;

    // Test a real PDF file.
    // If we find x-matlab because it starts with '%' then we are not ordering by priority.
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempFileName = tempFile.fileName();
    tempFile.write("%PDF-");
    tempFile.close();
    mime = db.mimeTypeForFile(tempFileName);
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    QFile file(tempFileName);
    mime = db.mimeTypeForData(&file); // QIODevice ctor
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    // by name only, we cannot find the mimetype
    mime = db.mimeTypeForFile(tempFileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QVERIFY(mime.isDefault());

    // Test the case where the extension doesn't match the contents: extension wins
    {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_QMimeDatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("%PDF-");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
        // fast mode finds the same
        mime = db.mimeTypeForFile(txtTempFileName, QMimeDatabase::MatchExtension);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
    }

    // Now the case where extension differs from contents, but contents has >80 magic rule
    // XDG spec says: contents wins. But we can't sniff all files...
    {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_QMimeDatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("<smil");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
        mime = db.mimeTypeForFile(txtTempFileName, QMimeDatabase::MatchContent);
        QCOMPARE(mime.name(), QString::fromLatin1("application/smil"));
    }

    // Test what happens with an incorrect path
    mime = db.mimeTypeForFile(QString::fromLatin1("file:///etc/passwd" /* incorrect code, use a path instead */));
    QVERIFY(mime.isDefault());

    // findByData when the device cannot be opened (e.g. a directory)
    QFile dir("/");
    mime = db.mimeTypeForData(&dir);
    QVERIFY(mime.isDefault());
}

void tst_QMimeDatabase::mimeTypeForUrl()
{
    QMimeDatabase db;
    QVERIFY(db.mimeTypeForUrl(QUrl::fromEncoded("http://foo/bar.png")).isDefault()); // HTTP can't know before downloading
    QCOMPARE(db.mimeTypeForUrl(QUrl::fromEncoded("ftp://foo/bar.png")).name(), QString::fromLatin1("image/png"));
    QCOMPARE(db.mimeTypeForUrl(QUrl::fromEncoded("ftp://foo/bar")).name(), QString::fromLatin1("application/octet-stream")); // unknown extension
    QCOMPARE(db.mimeTypeForUrl(QUrl("mailto:something@example.com")).name(), QString::fromLatin1("application/octet-stream")); // unknown
    QCOMPARE(db.mimeTypeForUrl(QUrl("mailto:something@polish.pl")).name(), QString::fromLatin1("application/octet-stream")); // unknown, NOT perl ;)
}

void tst_QMimeDatabase::mimeTypeForData_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("tnef data, needs smi >= 0.20") << QByteArray("\x78\x9f\x3e\x22") << "application/vnd.ms-tnef";
    QTest::newRow("PDF magic") << QByteArray("%PDF-") << "application/pdf";
    QTest::newRow("PHP, High-priority rule") << QByteArray("<?php") << "application/x-php";
    QTest::newRow("diff\\t") << QByteArray("diff\t") << "text/x-patch";
    QTest::newRow("unknown") << QByteArray("\001abc?}") << "application/octet-stream";
}

void tst_QMimeDatabase::mimeTypeForData()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForData(data).name(), expectedMimeTypeName);
    QBuffer buffer(&data);
    QCOMPARE(db.mimeTypeForData(&buffer).name(), expectedMimeTypeName);
    QVERIFY(!buffer.isOpen()); // initial state was restored

    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(db.mimeTypeForData(&buffer).name(), expectedMimeTypeName);
    QVERIFY(buffer.isOpen());
    QCOMPARE(buffer.pos(), qint64(0));
}

void tst_QMimeDatabase::mimeTypeForFileAndContent_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("plain text, no extension") << QString::fromLatin1("textfile") << QByteArray("Hello world") << "text/plain";
    QTest::newRow("plain text, unknown extension") << QString::fromLatin1("textfile.foo") << QByteArray("Hello world") << "text/plain";
    // Needs kde/mimetypes.xml
    //QTest::newRow("plain text, doc extension") << QString::fromLatin1("textfile.doc") << QByteArray("Hello world") << "text/plain";

    // If you get powerpoint instead, then you're hit by https://bugs.freedesktop.org/show_bug.cgi?id=435,
    // upgrade to shared-mime-info >= 0.22
    const QByteArray oleData("\320\317\021\340\241\261\032\341"); // same as \xD0\xCF\x11\xE0 \xA1\xB1\x1A\xE1
    QTest::newRow("msword file, unknown extension") << QString::fromLatin1("mswordfile") << oleData << "application/x-ole-storage";
    QTest::newRow("excel file, found by extension") << QString::fromLatin1("excelfile.xls") << oleData << "application/vnd.ms-excel";
    QTest::newRow("text.xls, found by extension, user is in control") << QString::fromLatin1("text.xls") << oleData << "application/vnd.ms-excel";
}

void tst_QMimeDatabase::mimeTypeForFileAndContent()
{
    QFETCH(QString, name);
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForFileNameAndData(name, data).name(), expectedMimeTypeName);

    QBuffer buffer(&data);
    QCOMPARE(db.mimeTypeForFileNameAndData(name, &buffer).name(), expectedMimeTypeName);
    QVERIFY(!buffer.isOpen()); // initial state was restored

    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(db.mimeTypeForFileNameAndData(name, &buffer).name(), expectedMimeTypeName);
    QVERIFY(buffer.isOpen());
    QCOMPARE(buffer.pos(), qint64(0));
}

void tst_QMimeDatabase::allMimeTypes()
{
    QMimeDatabase db;
    const QList<QMimeType> lst = db.allMimeTypes(); // does NOT include aliases
    QVERIFY(!lst.isEmpty());

    // Hardcoding this is the only way to check both providers find the same number of mimetypes.
    QCOMPARE(lst.count(), 661);

    foreach (const QMimeType &mime, lst) {
        const QString name = mime.name();
        QVERIFY(!name.isEmpty());
        QCOMPARE(name.count(QLatin1Char('/')), 1);
        const QMimeType lookedupMime = db.mimeTypeForName(name);
        QVERIFY(lookedupMime.isValid());
        QCOMPARE(lookedupMime.name(), name); // if this fails, you have an alias defined as a real mimetype too!
    }
}

void tst_QMimeDatabase::suffixes_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("patterns");
    QTest::addColumn<QString>("preferredSuffix");

    QTest::newRow("mimetype with a single pattern") << "application/pdf" << "*.pdf" << "pdf";
    QTest::newRow("mimetype with multiple patterns") << "application/x-kpresenter" << "*.kpr;*.kpt" << "kpr";
    QTest::newRow("jpeg") << "image/jpeg" << "*.jpe;*.jpg;*.jpeg" << "jpeg";
    //if (KMimeType::sharedMimeInfoVersion() > KDE_MAKE_VERSION(0, 60, 0)) {
        QTest::newRow("mimetype with many patterns") << "application/vnd.wordperfect" << "*.wp;*.wp4;*.wp5;*.wp6;*.wpd;*.wpp" << "wp";
    //}
    QTest::newRow("oasis text mimetype") << "application/vnd.oasis.opendocument.text" << "*.odt" << "odt";
    QTest::newRow("oasis presentation mimetype") << "application/vnd.oasis.opendocument.presentation" << "*.odp" << "odp";
    QTest::newRow("mimetype with multiple patterns") << "text/plain" << "*.asc;*.txt;*,v" << "txt";
    QTest::newRow("mimetype with uncommon pattern") << "text/x-readme" << "README*" << QString();
    QTest::newRow("mimetype with no patterns") << "application/x-ole-storage" << QString() << QString();
}

void tst_QMimeDatabase::suffixes()
{
    QFETCH(QString, mimeType);
    QFETCH(QString, patterns);
    QFETCH(QString, preferredSuffix);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    QVERIFY(mime.isValid());
    // Sort both lists; order is unreliable since shared-mime-info uses hashes internally.
    QStringList expectedPatterns = patterns.split(QLatin1Char(';'));
    expectedPatterns.sort();
    QStringList mimePatterns = mime.globPatterns();
    mimePatterns.sort();
    QCOMPARE(mimePatterns.join(QLatin1Char(';')), expectedPatterns.join(QLatin1Char(';')));
    QCOMPARE(mime.preferredSuffix(), preferredSuffix);
}

void tst_QMimeDatabase::knownSuffix()
{
    QMimeDatabase db;
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar")), QString::fromLatin1("tar"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bar.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar.bz2")), QString::fromLatin1("tar.bz2"));
}

void tst_QMimeDatabase::symlinkToFifo() // QTBUG-48529
{
#ifdef Q_OS_UNIX
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dir = tempDir.path();
    const QString fifo = dir + "/fifo";
    QCOMPARE(mkfifo(QFile::encodeName(fifo), 0006), 0);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForFile(fifo).name(), QString::fromLatin1("inode/fifo"));

    // Now make a symlink to the fifo
    const QString link = dir + "/link";
    QVERIFY(QFile::link(fifo, link));
    QCOMPARE(db.mimeTypeForFile(link).name(), QString::fromLatin1("inode/fifo"));

#else
    QSKIP("This test requires pipes and symlinks");
#endif
}

void tst_QMimeDatabase::findByFileName_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("mimeTypeName");
    QTest::addColumn<QString>("xFail");

    if (m_testSuite.isEmpty())
        QSKIP("shared-mime-info test suite not available.");

    const QString prefix = m_testSuite + QLatin1Char('/');
    const QString fileName = prefix + QLatin1String("list");
    QFile f(fileName);
    QVERIFY2(f.open(QIODevice::ReadOnly|QIODevice::Text),
             qPrintable(QString::fromLatin1("Cannot open %1: %2").arg(fileName, f.errorString())));

    QByteArray line(1024, Qt::Uninitialized);

    while (!f.atEnd()) {
        int len = f.readLine(line.data(), 1023);

        if (len <= 2 || line.at(0) == '#')
            continue;

        QString string = QString::fromLatin1(line.constData(), len - 1).trimmed();
        QStringList list = string.split(QLatin1Char(' '), QString::SkipEmptyParts);
        QVERIFY(list.size() >= 2);

        QString filePath = list.at(0);
        QString mimeTypeType = list.at(1);
        QString xFail;
        if (list.size() >= 3)
            xFail = list.at(2);

        QTest::newRow(filePath.toLatin1().constData())
                      << QString(prefix + filePath)
                      << mimeTypeType << xFail;
    }
}

void tst_QMimeDatabase::findByFileName()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;

    //qDebug() << Q_FUNC_INFO << filePath;

    const QMimeType resultMimeType(database.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension));
    if (resultMimeType.isValid()) {
        //qDebug() << Q_FUNC_INFO << "MIME type" << resultMimeType.name() << "has generic icon name" << resultMimeType.genericIconName() << "and icon name" << resultMimeType.iconName();

// Loading icons depend on the icon theme, we can't enable this test
#if 0
        QCOMPARE(resultMimeType.genericIconName(), QIcon::fromTheme(resultMimeType.genericIconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.genericIconName()).isNull(), qPrintable(resultMimeType.genericIconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.genericIconName()), qPrintable(resultMimeType.genericIconName()));

        QCOMPARE(resultMimeType.iconName(), QIcon::fromTheme(resultMimeType.iconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.iconName()).isNull(), qPrintable(resultMimeType.iconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.iconName()), qPrintable(resultMimeType.iconName()));
#endif
    }
    const QString resultMimeTypeName = resultMimeType.name();
    //qDebug() << Q_FUNC_INFO << "mimeTypeForFile() returned" << resultMimeTypeName;

    const bool failed = resultMimeTypeName != mimeTypeName;
    const bool shouldFail = (xFail.length() >= 1 && xFail.at(0) == QLatin1Char('x'));
    if (shouldFail != failed) {
        // Results are ambiguous when multiple MIME types have the same glob
        // -> accept the current result if the found MIME type actually
        // matches the file's extension.
        // TODO: a better file format in testfiles/list!
        const QMimeType foundMimeType = database.mimeTypeForName(resultMimeTypeName);
        QVERIFY2(resultMimeType == foundMimeType, qPrintable(resultMimeType.name() + QString::fromLatin1(" vs. ") + foundMimeType.name()));
        if (foundMimeType.isValid()) {
            const QString extension = QFileInfo(filePath).suffix();
            //qDebug() << Q_FUNC_INFO << "globPatterns:" << foundMimeType.globPatterns() << "- extension:" << QString() + "*." + extension;
            if (foundMimeType.globPatterns().contains(QString::fromLatin1("*.") + extension))
                return;
        }
    }
    if (shouldFail) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath), QMimeDatabase::MatchExtension);
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);
}

void tst_QMimeDatabase::findByData_data()
{
    findByFileName_data();
}

void tst_QMimeDatabase::findByData()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.read(16384);

    const QString resultMimeTypeName = database.mimeTypeForData(data).name();
    if (xFail.length() >= 2 && xFail.at(1) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    QFileInfo info(filePath);
    QString mimeForInfo = database.mimeTypeForFile(info, QMimeDatabase::MatchContent).name();
    QCOMPARE(mimeForInfo, resultMimeTypeName);
    QString mimeForFile = database.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
    QCOMPARE(mimeForFile, resultMimeTypeName);
}

void tst_QMimeDatabase::findByFile_data()
{
    findByFileName_data();
}

// Note: this test fails on "testcompress.z" when using a shared-mime-info older than 1.0.
// This because of commit 0f9a506069c in shared-mime-info, which fixed the writing of
// case-insensitive patterns into mime.cache.
void tst_QMimeDatabase::findByFile()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    const QString resultMimeTypeName = database.mimeTypeForFile(filePath).name();
    //qDebug() << Q_FUNC_INFO << filePath << "->" << resultMimeTypeName;
    if (xFail.length() >= 3 && xFail.at(2) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath));
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);
}


void tst_QMimeDatabase::fromThreads()
{
    QThreadPool tp;
    tp.setMaxThreadCount(20);
    // Note that data-based tests cannot be used here (QTest::fetchData asserts).
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::mimeTypeForName);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::aliases);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::allMimeTypes);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::icons);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::inheritance);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::knownSuffix);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::mimeTypeForFileWithContent);
    QtConcurrent::run(&tp, this, &tst_QMimeDatabase::allMimeTypes); // a second time
    QVERIFY(tp.waitForDone(60000));
}

#ifndef QT_NO_PROCESS
static bool runUpdateMimeDatabase(const QString &path) // TODO make it a QMimeDatabase method?
{
    const QString umdCommand = QString::fromLatin1("update-mime-database");
    const QString umd = QStandardPaths::findExecutable(umdCommand);
    if (umd.isEmpty()) {
        qWarning("%s does not exist.", qPrintable(umdCommand));
        return false;
    }

    QElapsedTimer timer;
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); // silence output
    qDebug().noquote() << "runUpdateMimeDatabase: running" << umd << path << "...";
    timer.start();
    proc.start(umd, QStringList(path));
    if (!proc.waitForStarted()) {
        qWarning("Cannot start %s: %s",
                 qPrintable(umd), qPrintable(proc.errorString()));
        return false;
    }
    const bool success = proc.waitForFinished();
    qDebug().noquote() << "runUpdateMimeDatabase: done,"
        << success << timer.elapsed() << "ms";
    return true;
}

static bool waitAndRunUpdateMimeDatabase(const QString &path)
{
    QFileInfo mimeCacheInfo(path + QString::fromLatin1("/mime.cache"));
    if (mimeCacheInfo.exists()) {
        // Wait until the beginning of the next second
        while (mimeCacheInfo.lastModified().secsTo(QDateTime::currentDateTime()) == 0) {
            QTest::qSleep(200);
        }
    }
    return runUpdateMimeDatabase(path);
}
#endif // !QT_NO_PROCESS

static void checkHasMimeType(const QString &mimeType)
{
    QMimeDatabase db;
    QVERIFY(db.mimeTypeForName(mimeType).isValid());

    bool found = false;
    foreach (const QMimeType &mt, db.allMimeTypes()) {
        if (mt.name() == mimeType) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

static void ignoreInvalidMimetypeWarnings(const QString &mimeDir)
{
    const QByteArray basePath = QFile::encodeName(mimeDir) + "/packages/";
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic1.xml\nInvalid magic rule value \"foo\"").constData());
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic2.xml\nInvalid magic rule mask \"ffff\"").constData());
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic3.xml\nInvalid magic rule mask size \"0xffff\"").constData());
}

QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT int qmime_secondsBetweenChecks; // see qmimeprovider.cpp
QT_END_NAMESPACE

void tst_QMimeDatabase::installNewGlobalMimeType()
{
#if !defined(USE_XDG_DATA_DIRS)
    QSKIP("This test requires XDG_DATA_DIRS");
#endif

#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    const QString mimeDir = m_globalXdgDir + QLatin1String("/mime");
    const QString destDir = mimeDir + QLatin1String("/packages/");
    if (!QFileInfo(destDir).isDir())
        QVERIFY(QDir(m_globalXdgDir).mkpath(destDir));

    QString errorMessage;
    for (int i = 0; i < m_additionalMimeFileNames.size(); ++i) {
        const QString destFile = destDir + m_additionalMimeFileNames.at(i);
        QFile::remove(destFile);
        QVERIFY2(copyResourceFile(m_additionalMimeFilePaths.at(i), destFile, &errorMessage), qPrintable(errorMessage));
    }
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(mimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");

    if (!m_isUsingCacheProvider)
        ignoreInvalidMimetypeWarnings(mimeDir);

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-SuSE-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    checkHasMimeType("text/x-suse-ymp");

    // Test that a double-definition of a mimetype doesn't lead to sniffing ("conflicting globs").
    const QString qmlTestFile = QLatin1String(RESOURCE_PREFIX "test.qml");
    QVERIFY2(!qmlTestFile.isEmpty(),
             qPrintable(QString::fromLatin1("Cannot find '%1' starting from '%2'").
                        arg("test.qml", QDir::currentPath())));
    QCOMPARE(db.mimeTypeForFile(qmlTestFile).name(),
             QString::fromLatin1("text/x-qml"));

    // ensure we can access the empty glob list
    {
        QMimeType objcsrc = db.mimeTypeForName(QStringLiteral("text/x-objcsrc"));
        QVERIFY(objcsrc.isValid());
        qDebug() << objcsrc.globPatterns();
    }

    // Now test removing the mimetype definitions again
    for (int i = 0; i < m_additionalMimeFileNames.size(); ++i)
        QFile::remove(destDir + m_additionalMimeFileNames.at(i));
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(mimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
#endif // !QT_NO_PROCESS
}

void tst_QMimeDatabase::installNewLocalMimeType()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;

    // Check that we're starting clean
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid());

    const QString destDir = m_localMimeDir + QLatin1String("/packages/");
    QVERIFY(QDir().mkpath(destDir));
    QString errorMessage;
    for (int i = 0; i < m_additionalMimeFileNames.size(); ++i) {
        const QString destFile = destDir + m_additionalMimeFileNames.at(i);
        QFile::remove(destFile);
        QVERIFY2(copyResourceFile(m_additionalMimeFilePaths.at(i), destFile, &errorMessage), qPrintable(errorMessage));
    }
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(m_localMimeDir)) {
        const QString skipWarning = QStringLiteral("shared-mime-info not found, skipping mime.cache test (")
                                    + QDir::toNativeSeparators(m_localMimeDir) + QLatin1Char(')');
        QSKIP(qPrintable(skipWarning));
    }

    if (!m_isUsingCacheProvider)
        ignoreInvalidMimetypeWarnings(m_localMimeDir);

    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    // These mimetypes have invalid magic, but still do exist.
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid());
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic2")).isValid());
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic3")).isValid());

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-SuSE-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    QCOMPARE(db.mimeTypeForName(QLatin1String("text/x-SuSE-ymu")).comment(), QString("URL of a YaST Meta Package"));
    checkHasMimeType("text/x-suse-ymp");

    // Test that a double-definition of a mimetype doesn't lead to sniffing ("conflicting globs").
    const QString qmlTestFile = QLatin1String(RESOURCE_PREFIX "test.qml");
    QVERIFY2(!qmlTestFile.isEmpty(),
             qPrintable(QString::fromLatin1("Cannot find '%1' starting from '%2'").
                        arg("test.qml", QDir::currentPath())));
    QCOMPARE(db.mimeTypeForFile(qmlTestFile).name(),
             QString::fromLatin1("text/x-qml"));

    // Now test removing the local mimetypes again (note, this leaves a mostly-empty mime.cache file)
    for (int i = 0; i < m_additionalMimeFileNames.size(); ++i)
        QFile::remove(destDir + m_additionalMimeFileNames.at(i));
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(m_localMimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    // And now the user goes wild and uses rm -rf
    QFile::remove(m_localMimeDir + QString::fromLatin1("/mime.cache"));
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
#endif
}

QTEST_GUILESS_MAIN(tst_QMimeDatabase)
